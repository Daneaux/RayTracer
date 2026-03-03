#pragma once

#include "renderer/IRenderer.h"
#include "DXDevice.h"
#include "math/MathTypes.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

using Microsoft::WRL::ComPtr;

struct ColorVertex {
    Vec3 position;
    Vec4 color;
};

class SceneOverviewRenderer : public IRenderer {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;
    void OnResize(DXDevice& device, uint32_t width, uint32_t height) override;
    void Render(DXDevice& device, const Scene& scene,
                const Camera& camera, SwapChainTarget& target) override;

    void SetObservedCamera(const Camera* cam) { m_observedCamera = cam; }

private:
    struct CBPerObject {
        float wvp[16];
    };

    void BuildGridGeometry(ID3D11Device* dev);
    void BuildSphereWireframe(ID3D11Device* dev);
    void BuildLightIndicator(ID3D11Device* dev);
    void BuildFrustumBuffers(ID3D11Device* dev);
    void UpdateFrustumGeometry(ID3D11DeviceContext* ctx);

    void DrawMesh(ID3D11DeviceContext* ctx, ID3D11Buffer* vb, ID3D11Buffer* ib,
                  uint32_t indexCount, const Mat4& wvp);

    const Camera* m_observedCamera = nullptr;

    // Static geometry
    ComPtr<ID3D11Buffer> m_gridVB, m_gridIB;
    uint32_t m_gridIndexCount = 0;

    ComPtr<ID3D11Buffer> m_sphereVB, m_sphereIB;
    uint32_t m_sphereIndexCount = 0;

    ComPtr<ID3D11Buffer> m_lightVB, m_lightIB;
    uint32_t m_lightIndexCount = 0;

    // Dynamic geometry
    ComPtr<ID3D11Buffer> m_frustumVB, m_frustumIB;
    uint32_t m_frustumIndexCount = 0;

    // Pipeline state
    ComPtr<ID3D11VertexShader>      m_vs;
    ComPtr<ID3D11PixelShader>       m_ps;
    ComPtr<ID3D11InputLayout>       m_layout;
    ComPtr<ID3D11Buffer>            m_cbPerObject;
    ComPtr<ID3D11RasterizerState>   m_rasterState;
    ComPtr<ID3D11DepthStencilState> m_depthState;
};
