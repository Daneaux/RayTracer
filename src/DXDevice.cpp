#include "DXDevice.h"
#include <d3d11.h>
#include <dxgi1_2.h>

bool DXDevice::Initialize() {
    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels, 1,
        D3D11_SDK_VERSION,
        &m_device,
        &m_featureLevel,
        &m_context
    );
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create D3D11 device\n");
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_device.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) return false;

    hr = adapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to get DXGI factory\n");
        return false;
    }

    return true;
}
