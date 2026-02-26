#include "renderer/FullscreenQuad.h"
#include <d3dcompiler.h>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

bool FullscreenQuad::Initialize(DXDevice& device, uint32_t width, uint32_t height) {
    CreateTextures(device, width, height);

    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    HRESULT hr = D3DCompileFromFile(
        L"shaders/FullscreenQuadVS.hlsl", nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob
    );
    if (FAILED(hr)) {
        if (errorBlob)
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        OutputDebugStringA("Failed to compile FullscreenQuadVS.hlsl\n");
        return false;
    }

    hr = D3DCompileFromFile(
        L"shaders/FullscreenQuadPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &psBlob, &errorBlob
    );
    if (FAILED(hr)) {
        if (errorBlob)
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        OutputDebugStringA("Failed to compile FullscreenQuadPS.hlsl\n");
        return false;
    }

    auto* dev = device.GetDevice();
    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    dev->CreateSamplerState(&sampDesc, &m_sampler);

    return true;
}

void FullscreenQuad::Resize(DXDevice& device, uint32_t width, uint32_t height) {
    if (width == m_texWidth && height == m_texHeight) return;
    CreateTextures(device, width, height);
}

void FullscreenQuad::Upload(DXDevice& device, const uint32_t* pixels, uint32_t w, uint32_t h) {
    if (w != m_texWidth || h != m_texHeight) {
        CreateTextures(device, w, h);
    }

    auto* ctx = device.GetContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->Map(m_stagingTexture.Get(), 0, D3D11_MAP_WRITE, 0, &mapped);
    if (FAILED(hr)) return;

    const uint32_t srcRowBytes = w * sizeof(uint32_t);
    for (uint32_t row = 0; row < h; ++row) {
        memcpy(
            static_cast<uint8_t*>(mapped.pData) + row * mapped.RowPitch,
            reinterpret_cast<const uint8_t*>(pixels) + row * srcRowBytes,
            srcRowBytes
        );
    }

    ctx->Unmap(m_stagingTexture.Get(), 0);
    ctx->CopyResource(m_displayTexture.Get(), m_stagingTexture.Get());
}

void FullscreenQuad::Draw(DXDevice& device) {
    auto* ctx = device.GetContext();

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

    ctx->VSSetShader(m_vs.Get(), nullptr, 0);
    ctx->PSSetShader(m_ps.Get(), nullptr, 0);

    ID3D11ShaderResourceView* srvs[] = {m_srv.Get()};
    ctx->PSSetShaderResources(0, 1, srvs);

    ID3D11SamplerState* samplers[] = {m_sampler.Get()};
    ctx->PSSetSamplers(0, 1, samplers);

    ctx->Draw(3, 0);

    // Unbind SRV to avoid warnings
    ID3D11ShaderResourceView* nullSrv[] = {nullptr};
    ctx->PSSetShaderResources(0, 1, nullSrv);
}

void FullscreenQuad::CreateTextures(DXDevice& device, uint32_t w, uint32_t h) {
    m_stagingTexture.Reset();
    m_displayTexture.Reset();
    m_srv.Reset();

    m_texWidth = w;
    m_texHeight = h;

    auto* dev = device.GetDevice();

    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = w;
    stagingDesc.Height = h;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTexture);

    D3D11_TEXTURE2D_DESC displayDesc = {};
    displayDesc.Width = w;
    displayDesc.Height = h;
    displayDesc.MipLevels = 1;
    displayDesc.ArraySize = 1;
    displayDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    displayDesc.SampleDesc.Count = 1;
    displayDesc.Usage = D3D11_USAGE_DEFAULT;
    displayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    dev->CreateTexture2D(&displayDesc, nullptr, &m_displayTexture);

    dev->CreateShaderResourceView(m_displayTexture.Get(), nullptr, &m_srv);
}
