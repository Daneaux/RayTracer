#include "renderer/SceneOverviewRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

static const float PI = 3.14159265358979323846f;

// Helper to create a static vertex/index buffer pair
static bool CreateStaticBuffers(ID3D11Device* dev,
                                const std::vector<ColorVertex>& verts,
                                const std::vector<uint32_t>& indices,
                                ComPtr<ID3D11Buffer>& vb,
                                ComPtr<ID3D11Buffer>& ib) {
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(sizeof(ColorVertex) * verts.size());
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = verts.data();
    if (FAILED(dev->CreateBuffer(&vbDesc, &vbData, &vb))) return false;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();
    if (FAILED(dev->CreateBuffer(&ibDesc, &ibData, &ib))) return false;

    return true;
}

bool SceneOverviewRenderer::Initialize(DXDevice& device, uint32_t width, uint32_t height) {
    (void)width; (void)height;
    auto* dev = device.GetDevice();

    // Compile shaders
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    HRESULT hr = D3DCompileFromFile(
        L"shaders/OverviewVS.hlsl", nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob
    );
    if (FAILED(hr)) {
        if (errorBlob) OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        OutputDebugStringA("Failed to compile OverviewVS.hlsl\n");
        return false;
    }

    hr = D3DCompileFromFile(
        L"shaders/OverviewPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &psBlob, &errorBlob
    );
    if (FAILED(hr)) {
        if (errorBlob) OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        OutputDebugStringA("Failed to compile OverviewPS.hlsl\n");
        return false;
    }

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    // Input layout: POSITION (float3) + COLOR (float4)
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vec3), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    dev->CreateInputLayout(layoutDesc, 2,
                           vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                           &m_layout);

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CBPerObject);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    dev->CreateBuffer(&cbDesc, nullptr, &m_cbPerObject);

    // Rasterizer state (no backface culling for wireframe visibility)
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    rsDesc.AntialiasedLineEnable = TRUE;
    dev->CreateRasterizerState(&rsDesc, &m_rasterState);

    // Depth state
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dev->CreateDepthStencilState(&dsDesc, &m_depthState);

    // Build geometry
    BuildGridGeometry(dev);
    BuildSphereWireframe(dev);
    BuildLightIndicator(dev);
    BuildFrustumBuffers(dev);

    return true;
}

void SceneOverviewRenderer::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    (void)device; (void)width; (void)height;
}

void SceneOverviewRenderer::DrawMesh(ID3D11DeviceContext* ctx, ID3D11Buffer* vb,
                                      ID3D11Buffer* ib, uint32_t indexCount,
                                      const Mat4& wvp) {
    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    ctx->Map(m_cbPerObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(((CBPerObject*)mapped.pData)->wvp, &wvp.m, 64);
    ctx->Unmap(m_cbPerObject.Get(), 0);

    UINT stride = sizeof(ColorVertex);
    UINT offset = 0;
    ID3D11Buffer* vbs[] = {vb};
    ctx->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
    ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    ctx->DrawIndexed(indexCount, 0, 0);
}

void SceneOverviewRenderer::Render(DXDevice& device, const Scene& scene,
                                    const Camera& camera, SwapChainTarget& target) {
    auto* ctx = device.GetContext();

    float clearColor[4] = {0.05f, 0.05f, 0.08f, 1.0f};
    target.Bind(device);
    target.Clear(device, clearColor);

    // Set pipeline state
    ctx->IASetInputLayout(m_layout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ctx->RSSetState(m_rasterState.Get());
    ctx->OMSetDepthStencilState(m_depthState.Get(), 0);
    ctx->VSSetShader(m_vs.Get(), nullptr, 0);
    ctx->PSSetShader(m_ps.Get(), nullptr, 0);

    ID3D11Buffer* cbs[] = {m_cbPerObject.Get()};
    ctx->VSSetConstantBuffers(0, 1, cbs);

    Mat4 vp = camera.GetViewProjection();

    // temp: get one sphere
    WorldObject& sphereObj = *scene.GetObjects()[0];
    const SphereObject& sphere = dynamic_cast<SphereObject&>(sphereObj);
    const Material& mat = sphere.GetMaterial();

    Light* l = scene.GetLights()[0];
    PointLight& light = dynamic_cast<PointLight&>(*l);

    // 1. Draw grid (world space, identity world)
    DrawMesh(ctx, m_gridVB.Get(), m_gridIB.Get(), m_gridIndexCount, vp);

    // 2. Draw scene sphere wireframe
    Mat4 sphereWorld = Mat4::Scaling(sphere.GetRadius());// *Mat4::Translation(scene.sphere.center);
    DrawMesh(ctx, m_sphereVB.Get(), m_sphereIB.Get(), m_sphereIndexCount, sphereWorld * vp);

    // 3. Draw light indicator
    Mat4 lightWorld = Mat4::Scaling(0.15f) * Mat4::Translation(light.position);
    DrawMesh(ctx, m_lightVB.Get(), m_lightIB.Get(), m_lightIndexCount, lightWorld * vp);

    // 4. Draw camera frustum (if observed camera is set)
    if (m_observedCamera) {
        UpdateFrustumGeometry(ctx);
        DrawMesh(ctx, m_frustumVB.Get(), m_frustumIB.Get(), m_frustumIndexCount, vp);
    }
}

// ---------------------------------------------------------------
// Grid geometry: 3 colored planes + axis lines
// ---------------------------------------------------------------
void SceneOverviewRenderer::BuildGridGeometry(ID3D11Device* dev) {
    std::vector<ColorVertex> verts;
    std::vector<uint32_t> indices;

    auto addLine = [&](const Vec3& a, const Vec3& b, const Vec4& color) {
        uint32_t base = (uint32_t)verts.size();
        verts.push_back({a, color});
        verts.push_back({b, color});
        indices.push_back(base);
        indices.push_back(base + 1);
    };

    const float gridSize = 5.0f;
    const float step = 1.0f;

    // XZ plane (Y=0) - gray
    Vec4 xzColor = {0.3f, 0.3f, 0.3f, 1.0f};
    for (float i = -gridSize; i <= gridSize; i += step) {
        if (std::abs(i) < 0.01f) continue; // skip origin lines (drawn as axes)
        addLine({i, 0, -gridSize}, {i, 0, gridSize}, xzColor);
        addLine({-gridSize, 0, i}, {gridSize, 0, i}, xzColor);
    }

    // XY plane (Z=0) - faint blue
    Vec4 xyColor = {0.15f, 0.15f, 0.35f, 1.0f};
    for (float i = -gridSize; i <= gridSize; i += step) {
        if (std::abs(i) < 0.01f) continue;
        addLine({i, -gridSize, 0}, {i, gridSize, 0}, xyColor);
        addLine({-gridSize, i, 0}, {gridSize, i, 0}, xyColor);
    }

    // YZ plane (X=0) - faint red
    Vec4 yzColor = {0.35f, 0.15f, 0.15f, 1.0f};
    for (float i = -gridSize; i <= gridSize; i += step) {
        if (std::abs(i) < 0.01f) continue;
        addLine({0, i, -gridSize}, {0, i, gridSize}, yzColor);
        addLine({0, -gridSize, i}, {0, gridSize, i}, yzColor);
    }

    // Axis lines (bright, extend beyond grid)
    float axisLen = gridSize;
    addLine({-axisLen, 0, 0}, {axisLen, 0, 0}, {1.0f, 0.2f, 0.2f, 1.0f}); // X = red
    addLine({0, -axisLen, 0}, {0, axisLen, 0}, {0.2f, 1.0f, 0.2f, 1.0f}); // Y = green
    addLine({0, 0, -axisLen}, {0, 0, axisLen}, {0.2f, 0.2f, 1.0f, 1.0f}); // Z = blue

    m_gridIndexCount = (uint32_t)indices.size();
    CreateStaticBuffers(dev, verts, indices, m_gridVB, m_gridIB);
}

// ---------------------------------------------------------------
// Wireframe sphere (unit sphere at origin, LINE_LIST edges)
// ---------------------------------------------------------------
void SceneOverviewRenderer::BuildSphereWireframe(ID3D11Device* dev) {
    std::vector<ColorVertex> verts;
    std::vector<uint32_t> indices;

    const uint32_t slices = 16;
    const uint32_t stacks = 8;
    Vec4 color = {0.8f, 0.3f, 0.3f, 1.0f}; // will appear in sphere's actual color via tint later

    // Generate vertices on unit sphere
    for (uint32_t i = 0; i <= stacks; ++i) {
        float phi = PI * (float)i / (float)stacks;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (uint32_t j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * (float)j / (float)slices;
            Vec3 pos = {sinPhi * std::cos(theta), cosPhi, sinPhi * std::sin(theta)};
            verts.push_back({pos, color});
        }
    }

    // Generate line indices (horizontal rings + vertical arcs)
    for (uint32_t i = 0; i <= stacks; ++i) {
        for (uint32_t j = 0; j < slices; ++j) {
            uint32_t curr = i * (slices + 1) + j;
            uint32_t next = curr + 1;
            // Horizontal ring edge
            indices.push_back(curr);
            indices.push_back(next);
        }
    }
    for (uint32_t j = 0; j <= slices; ++j) {
        for (uint32_t i = 0; i < stacks; ++i) {
            uint32_t curr = i * (slices + 1) + j;
            uint32_t below = curr + (slices + 1);
            // Vertical arc edge
            indices.push_back(curr);
            indices.push_back(below);
        }
    }

    m_sphereIndexCount = (uint32_t)indices.size();
    CreateStaticBuffers(dev, verts, indices, m_sphereVB, m_sphereIB);
}

// ---------------------------------------------------------------
// Light indicator: octahedron at origin
// ---------------------------------------------------------------
void SceneOverviewRenderer::BuildLightIndicator(ID3D11Device* dev) {
    Vec4 color = {1.0f, 1.0f, 0.0f, 1.0f}; // yellow

    // 6 vertices of an octahedron (unit size)
    std::vector<ColorVertex> verts = {
        {{1, 0, 0},  color},  // 0: +X
        {{-1, 0, 0}, color},  // 1: -X
        {{0, 1, 0},  color},  // 2: +Y
        {{0, -1, 0}, color},  // 3: -Y
        {{0, 0, 1},  color},  // 4: +Z
        {{0, 0, -1}, color},  // 5: -Z
    };

    // 12 edges of the octahedron
    std::vector<uint32_t> indices = {
        // Top edges (from +Y to equator)
        2, 0,  2, 4,  2, 1,  2, 5,
        // Bottom edges (from -Y to equator)
        3, 0,  3, 4,  3, 1,  3, 5,
        // Equator edges
        0, 4,  4, 1,  1, 5,  5, 0,
    };

    m_lightIndexCount = (uint32_t)indices.size();
    CreateStaticBuffers(dev, verts, indices, m_lightVB, m_lightIB);
}

// ---------------------------------------------------------------
// Frustum: dynamic VB, static IB
// ---------------------------------------------------------------
void SceneOverviewRenderer::BuildFrustumBuffers(ID3D11Device* dev) {
    // 9 vertices: eye + 4 near corners + 4 far corners
    // Create dynamic vertex buffer with max capacity
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(ColorVertex) * 9;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&vbDesc, nullptr, &m_frustumVB);

    // Static index buffer:
    // 0 = eye
    // 1-4 = near corners (TL, TR, BR, BL)
    // 5-8 = far corners (TL, TR, BR, BL)
    std::vector<uint32_t> indices = {
        // Eye to near corners
        0, 1,  0, 2,  0, 3,  0, 4,
        // Near rectangle
        1, 2,  2, 3,  3, 4,  4, 1,
        // Far rectangle
        5, 6,  6, 7,  7, 8,  8, 5,
        // Near-to-far connecting edges
        1, 5,  2, 6,  3, 7,  4, 8,
    };

    m_frustumIndexCount = (uint32_t)indices.size();

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();
    dev->CreateBuffer(&ibDesc, &ibData, &m_frustumIB);
}

void SceneOverviewRenderer::UpdateFrustumGeometry(ID3D11DeviceContext* ctx) {
    if (!m_observedCamera) return;

    const Camera& cam = *m_observedCamera;
    Vec3 eye = cam.GetPosition();
    Vec3 fwd = cam.GetForward();
    float fovY = cam.GetFovY();
    float aspect = cam.GetAspectRatio();
    float nearZ = cam.GetNearZ();
    float farZ = std::min(cam.GetFarZ(), 5.0f); // cap for visibility

    Vec3 right = Vec3::Cross(fwd, Vec3(0, 1, 0)).Normalized();
    Vec3 camUp = Vec3::Cross(right, fwd).Normalized();

    float nearH = std::tan(fovY * 0.5f) * nearZ;
    float nearW = nearH * aspect;
    float farH = std::tan(fovY * 0.5f) * farZ;
    float farW = farH * aspect;

    Vec3 nearCenter = eye + fwd * nearZ;
    Vec3 farCenter = eye + fwd * farZ;

    Vec4 eyeColor = {1.0f, 1.0f, 0.0f, 1.0f};    // yellow for eye point
    Vec4 nearColor = {0.0f, 1.0f, 1.0f, 1.0f};    // cyan for near plane
    Vec4 farColor = {0.0f, 0.7f, 0.7f, 1.0f};     // darker cyan for far

    ColorVertex verts[9];
    // Eye point
    verts[0] = {eye, eyeColor};
    // Near plane corners: TL, TR, BR, BL
    verts[1] = {nearCenter + camUp * nearH - right * nearW, nearColor};
    verts[2] = {nearCenter + camUp * nearH + right * nearW, nearColor};
    verts[3] = {nearCenter - camUp * nearH + right * nearW, nearColor};
    verts[4] = {nearCenter - camUp * nearH - right * nearW, nearColor};
    // Far plane corners: TL, TR, BR, BL
    verts[5] = {farCenter + camUp * farH - right * farW, farColor};
    verts[6] = {farCenter + camUp * farH + right * farW, farColor};
    verts[7] = {farCenter - camUp * farH + right * farW, farColor};
    verts[8] = {farCenter - camUp * farH - right * farW, farColor};

    D3D11_MAPPED_SUBRESOURCE mapped;
    ctx->Map(m_frustumVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, verts, sizeof(verts));
    ctx->Unmap(m_frustumVB.Get(), 0);
}
