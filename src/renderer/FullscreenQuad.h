#pragma once

#include "DXDevice.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class FullscreenQuad {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height);
    void Resize(DXDevice& device, uint32_t width, uint32_t height);
    void Upload(DXDevice& device, const uint32_t* pixels, uint32_t w, uint32_t h);
    void Draw(DXDevice& device);

private:
    void CreateTextures(DXDevice& device, uint32_t w, uint32_t h);

    ComPtr<ID3D11Texture2D>          m_stagingTexture;
    ComPtr<ID3D11Texture2D>          m_displayTexture;
    ComPtr<ID3D11ShaderResourceView> m_srv;
    ComPtr<ID3D11SamplerState>       m_sampler;
    ComPtr<ID3D11VertexShader>       m_vs;
    ComPtr<ID3D11PixelShader>        m_ps;
    uint32_t m_texWidth = 0;
    uint32_t m_texHeight = 0;
};
