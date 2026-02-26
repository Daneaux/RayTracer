#pragma once

#include "renderer/IRenderer.h"
#include "DXDevice.h"
#include "math/MathTypes.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class GPURenderer : public IRenderer {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;
    void OnResize(DXDevice& device, uint32_t width, uint32_t height) override;
    void Render(DXDevice& device, const Scene& scene,
                const Camera& camera, SwapChainTarget& target) override;

private:
    struct Vertex {
        Vec3 position;
        Vec3 normal;
    };

    struct CBPerObject {
        float world[16];
        float worldViewProj[16];
        float worldInverseTranspose[16];
    };

    struct CBPerFrame {
        Vec4 lightPosition;
        Vec4 lightColor;
        Vec4 cameraPosition;
        Vec4 ambientColor;
        Vec4 sphereColor;
    };

    void GenerateUVSphere(float radius, uint32_t slices, uint32_t stacks,
                          std::vector<Vertex>& vertices,
                          std::vector<uint32_t>& indices);

    ComPtr<ID3D11Buffer>            m_vertexBuffer;
    ComPtr<ID3D11Buffer>            m_indexBuffer;
    ComPtr<ID3D11Buffer>            m_cbPerObject;
    ComPtr<ID3D11Buffer>            m_cbPerFrame;
    ComPtr<ID3D11VertexShader>      m_vertexShader;
    ComPtr<ID3D11PixelShader>       m_pixelShader;
    ComPtr<ID3D11InputLayout>       m_inputLayout;
    ComPtr<ID3D11RasterizerState>   m_rasterState;
    ComPtr<ID3D11DepthStencilState> m_depthState;
    ComPtr<ID3D11VertexShader>      m_gradientVS;
    ComPtr<ID3D11PixelShader>       m_gradientPS;
    ComPtr<ID3D11DepthStencilState> m_noDepthState;
    uint32_t m_indexCount = 0;
};
