#pragma once

#include <cstdint>

class DXDevice;
class SwapChainTarget;
class Camera;
class Scene;

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize(DXDevice& device, uint32_t width, uint32_t height) = 0;
    virtual void OnResize(DXDevice& device, uint32_t width, uint32_t height) = 0;
    virtual void Render(
        DXDevice& device, 
        const Scene& scene,
        const Camera& camera,
        SwapChainTarget& target) = 0;
};
