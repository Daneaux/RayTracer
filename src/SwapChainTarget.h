#pragma once

#include "DXDevice.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

class SwapChainTarget {
public:
    bool Create(DXDevice& device, HWND hwnd, uint32_t width, uint32_t height);
    void Resize(DXDevice& device, uint32_t width, uint32_t height);
    void Bind(DXDevice& device);
    void Clear(DXDevice& device, const float color[4]);
    void Present();

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

private:
    void CreateRenderTargetAndDepth(DXDevice& device);

    ComPtr<IDXGISwapChain1>         m_swapChain;
    ComPtr<ID3D11RenderTargetView>  m_rtv;
    ComPtr<ID3D11DepthStencilView>  m_dsv;
    ComPtr<ID3D11Texture2D>         m_depthBuffer;
    D3D11_VIEWPORT                  m_viewport = {};
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
