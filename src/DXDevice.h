#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DXDevice {
public:
    bool Initialize();

    ID3D11Device*        GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext*  GetContext() const { return m_context.Get(); }
    IDXGIFactory2*       GetDXGIFactory() const { return m_dxgiFactory.Get(); }

private:
    ComPtr<ID3D11Device>        m_device;
    ComPtr<ID3D11DeviceContext>  m_context;
    ComPtr<IDXGIFactory2>       m_dxgiFactory;
    D3D_FEATURE_LEVEL           m_featureLevel = D3D_FEATURE_LEVEL_11_0;
};
