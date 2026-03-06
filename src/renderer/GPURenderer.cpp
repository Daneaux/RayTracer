#include "renderer/GPURenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstring>
#include "../lights.h"


#pragma comment(lib, "d3dcompiler.lib")

static const float PI = 3.14159265358979323846f;

bool GPURenderer::Initialize(DXDevice& device, uint32_t width, uint32_t height) {
    auto* dev = device.GetDevice();

    // Compile shaders
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    HRESULT hr = D3DCompileFromFile(
        L"shaders/PhongVS.hlsl", nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob
    );
    if (FAILED(hr)) {
        if (errorBlob)
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        OutputDebugStringA("Failed to compile PhongVS.hlsl\n");
        return false;
    }

    hr = D3DCompileFromFile(
        L"shaders/PhongPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &psBlob, &errorBlob
    );
    if (FAILED(hr)) {
        if (errorBlob)
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        OutputDebugStringA("Failed to compile PhongPS.hlsl\n");
        return false;
    }

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    dev->CreateInputLayout(layout, 2,
                           vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                           &m_inputLayout);

    // Generate sphere mesh
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    GenerateUVSphere(1.0f, 32, 16, vertices, indices);
    m_indexCount = (uint32_t)indices.size();

    // Vertex buffer
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(sizeof(Vertex) * vertices.size());
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();
    dev->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);

    // Index buffer
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();
    dev->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);

    // Constant buffers
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    cbDesc.ByteWidth = sizeof(CBPerObject);
    dev->CreateBuffer(&cbDesc, nullptr, &m_cbPerObject);

    cbDesc.ByteWidth = sizeof(CBPerFrame);
    dev->CreateBuffer(&cbDesc, nullptr, &m_cbPerFrame);

    // Rasterizer state
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    dev->CreateRasterizerState(&rsDesc, &m_rasterState);

    // Depth stencil state
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dev->CreateDepthStencilState(&dsDesc, &m_depthState);

    // No-depth state for gradient background pass
    D3D11_DEPTH_STENCIL_DESC noDepthDesc = {};
    noDepthDesc.DepthEnable = FALSE;
    noDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dev->CreateDepthStencilState(&noDepthDesc, &m_noDepthState);

    // Compile gradient background shaders (reuse FullscreenQuadVS + new GradientPS)
    {
        ComPtr<ID3DBlob> gradVsBlob, gradPsBlob, gradErrorBlob;

        hr = D3DCompileFromFile(
            L"shaders/FullscreenQuadVS.hlsl", nullptr, nullptr,
            "main", "vs_5_0", 0, 0, &gradVsBlob, &gradErrorBlob
        );
        if (FAILED(hr)) {
            if (gradErrorBlob)
                OutputDebugStringA((const char*)gradErrorBlob->GetBufferPointer());
            OutputDebugStringA("Failed to compile FullscreenQuadVS.hlsl for gradient\n");
            return false;
        }

        hr = D3DCompileFromFile(
            L"shaders/GradientPS.hlsl", nullptr, nullptr,
            "main", "ps_5_0", 0, 0, &gradPsBlob, &gradErrorBlob
        );
        if (FAILED(hr)) {
            if (gradErrorBlob)
                OutputDebugStringA((const char*)gradErrorBlob->GetBufferPointer());
            OutputDebugStringA("Failed to compile GradientPS.hlsl\n");
            return false;
        }

        dev->CreateVertexShader(gradVsBlob->GetBufferPointer(), gradVsBlob->GetBufferSize(), nullptr, &m_gradientVS);
        dev->CreatePixelShader(gradPsBlob->GetBufferPointer(), gradPsBlob->GetBufferSize(), nullptr, &m_gradientPS);
    }

    return true;
}

void GPURenderer::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    // Nothing to do - viewport is set by SwapChainTarget::Bind, projection updated by camera
    (void)device;
    (void)width;
    (void)height;
}

void GPURenderer::Render(
    DXDevice& device, 
    const Scene& scene,
    const Camera& camera, 
    SwapChainTarget& target)
{
    auto* ctx = device.GetContext();

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    target.Bind(device);
    target.Clear(device, clearColor);

    // --- Gradient background pass ---
    ctx->OMSetDepthStencilState(m_noDepthState.Get(), 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(m_gradientVS.Get(), nullptr, 0);
    ctx->PSSetShader(m_gradientPS.Get(), nullptr, 0);
    ctx->Draw(3, 0);

    // --- Sphere pass ---
    // Build matrices
    WorldObject* sphereObj = scene.GetObjects()[0];
    SphereObject* sphere = dynamic_cast<SphereObject*>(sphereObj);
    if (!sphere) {
        // Handle error: object is not a SphereObject
        return;
    }

	Light* l = scene.GetLights()[0];
	PointLight& light = dynamic_cast<PointLight&>(*l);

    Mat4 world = Mat4::Scaling(sphere->GetRadius()); //* Mat4::Translation(sphere.center);
    Mat4 vp = camera.GetViewProjection();
    Mat4 wvp = world * vp;
    Mat4 wit = world.Inverted().Transposed();

    // Update per-object CB
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        ctx->Map(m_cbPerObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        CBPerObject* cb = (CBPerObject*)mapped.pData;
        memcpy(cb->world, &world.m, 64);
        memcpy(cb->worldViewProj, &wvp.m, 64);
        memcpy(cb->worldInverseTranspose, &wit.m, 64);
        ctx->Unmap(m_cbPerObject.Get(), 0);
    }

    // Update per-frame CB
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        ctx->Map(m_cbPerFrame.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        CBPerFrame* cb = (CBPerFrame*)mapped.pData;
        cb->lightPosition = Vec4(light.position, 0.0f);
        cb->lightColor = Vec4(light.color, light.intensity);
        cb->cameraPosition = Vec4(camera.GetPosition(), 0.0f);
        cb->ambientColor = Vec4(scene.GetAmbientColor(), 0.0f);
		const Material& mat = sphere->GetMaterial();
        cb->sphereColor = Vec4(mat.diffuseColor, mat.specularReflection);
        ctx->Unmap(m_cbPerFrame.Get(), 0);
    }

    // Set pipeline state
    ctx->RSSetState(m_rasterState.Get());
    ctx->OMSetDepthStencilState(m_depthState.Get(), 0);

    ctx->IASetInputLayout(m_inputLayout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ID3D11Buffer* vbs[] = {m_vertexBuffer.Get()};
    ctx->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
    ctx->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    ctx->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    ctx->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    ID3D11Buffer* vsCbs[] = {m_cbPerObject.Get()};
    ctx->VSSetConstantBuffers(0, 1, vsCbs);

    ID3D11Buffer* psCbs[] = {m_cbPerFrame.Get()};
    ctx->PSSetConstantBuffers(1, 1, psCbs);

    ctx->DrawIndexed(m_indexCount, 0, 0);
}

void GPURenderer::GenerateUVSphere(float radius, uint32_t slices, uint32_t stacks,
                                    std::vector<Vertex>& vertices,
                                    std::vector<uint32_t>& indices) {
    vertices.clear();
    indices.clear();

    // Generate vertices
    for (uint32_t i = 0; i <= stacks; ++i) {
        float phi = PI * (float)i / (float)stacks;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (uint32_t j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * (float)j / (float)slices;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            Vertex v;
            v.normal = {sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
            v.position = v.normal * radius;
            vertices.push_back(v);
        }
    }

    // Generate indices
    for (uint32_t i = 0; i < stacks; ++i) {
        for (uint32_t j = 0; j < slices; ++j) {
            uint32_t topLeft = i * (slices + 1) + j;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = topLeft + (slices + 1);
            uint32_t bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}
