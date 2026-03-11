#pragma once

#include "renderer/IRenderer.h"
#include "renderer/FullscreenQuad.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <random>

class SoftwareRendererBase : public IRenderer {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;
    void OnResize(DXDevice& device, uint32_t width, uint32_t height) override;
    void Render(DXDevice& device, Scene& scene,
                const Camera& camera, SwapChainTarget& target) override;
    void Invalidate() override { m_dirty = true; }
    bool IsRenderComplete() const override { return m_renderComplete; }

protected:
    // Subclasses implement this to compute the color for a single pixel.
    // Return color with components in [0,1].
    virtual Vec3 ShadePixel(uint32_t px, uint32_t py, uint32_t w, uint32_t h,
                            const Camera& camera, Scene& scene) = 0;

    // Ensure pixel buffer and quad match target dimensions. Returns true if resized.
    bool EnsureBufferSize(DXDevice& device, uint32_t w, uint32_t h);

    // Upload pixel buffer and draw fullscreen quad to swap chain target.
    void UploadAndDraw(DXDevice& device, SwapChainTarget& target);

    // Pixel buffer (accessible to subclasses that override Render for custom drawing)
    std::vector<uint32_t> m_pixelBuffer;
    uint32_t m_bufWidth = 0;
    uint32_t m_bufHeight = 0;
    std::unique_ptr<FullscreenQuad> m_quad;

private:
    // Progressive rendering state
    std::vector<uint32_t> m_pixelOrder;       // shuffled pixel indices
    uint32_t              m_nextPixelIdx = 0; // position in shuffled list
    bool                  m_renderComplete = false;
    bool                  m_dirty = true;     // starts dirty to trigger first render
    std::mt19937          m_rng{std::random_device{}()};
};
