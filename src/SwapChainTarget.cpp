#include "SwapChainTarget.h"

bool SwapChainTarget::Create(DXDevice& device, HWND hwnd, uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr = device.GetDXGIFactory()->CreateSwapChainForHwnd(
        device.GetDevice(), hwnd, &desc, nullptr, nullptr, &m_swapChain
    );
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create swap chain\n");
        return false;
    }

    CreateRenderTargetAndDepth(device);
    return true;
}

void SwapChainTarget::Resize(DXDevice& device, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;

    m_rtv.Reset();
    m_dsv.Reset();
    m_depthBuffer.Reset();

    auto* ctx = device.GetContext();
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->Flush();

    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to resize swap chain buffers\n");
        return;
    }

    m_width = width;
    m_height = height;
    CreateRenderTargetAndDepth(device);
}

void SwapChainTarget::Bind(DXDevice& device) {
    auto* ctx = device.GetContext();
    ID3D11RenderTargetView* rtvs[] = {m_rtv.Get()};
    ctx->OMSetRenderTargets(1, rtvs, m_dsv.Get());
    ctx->RSSetViewports(1, &m_viewport);
}

void SwapChainTarget::Clear(DXDevice& device, const float color[4]) {
    auto* ctx = device.GetContext();
    ctx->ClearRenderTargetView(m_rtv.Get(), color);
    ctx->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void SwapChainTarget::Present() {
    m_swapChain->Present(1, 0);
}

void SwapChainTarget::CreateRenderTargetAndDepth(DXDevice& device) {
    ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device.GetDevice()->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_rtv);

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    device.GetDevice()->CreateTexture2D(&depthDesc, nullptr, &m_depthBuffer);
    device.GetDevice()->CreateDepthStencilView(m_depthBuffer.Get(), nullptr, &m_dsv);

    m_viewport = {0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f};
}
