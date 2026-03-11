#include "renderer/SoftwareRendererBase.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <algorithm>

bool SoftwareRendererBase::Initialize(DXDevice& device, uint32_t width, uint32_t height) {
    m_bufWidth = width;
    m_bufHeight = height;
    m_pixelBuffer.resize(width * height, 0xFF000000);

    m_quad = std::make_unique<FullscreenQuad>();
    return m_quad->Initialize(device, width, height);
}

void SoftwareRendererBase::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    (void)device;
    (void)width;
    (void)height;
    m_dirty = true;
}

bool SoftwareRendererBase::EnsureBufferSize(DXDevice& device, uint32_t w, uint32_t h) {
    if (w == m_bufWidth && h == m_bufHeight)
        return false;

    m_bufWidth = w;
    m_bufHeight = h;
    m_pixelBuffer.resize(w * h);
    m_quad->Resize(device, w, h);
    m_dirty = true;
    return true;
}

void SoftwareRendererBase::UploadAndDraw(DXDevice& device, SwapChainTarget& target) {
    m_quad->Upload(device, m_pixelBuffer.data(), m_bufWidth, m_bufHeight);

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    target.Bind(device);
    target.Clear(device, clearColor);
    m_quad->Draw(device);
}

void SoftwareRendererBase::Render(DXDevice& device, Scene& scene,
                                   const Camera& camera, SwapChainTarget& target) {
    uint32_t w = target.GetWidth();
    uint32_t h = target.GetHeight();
    if (w == 0 || h == 0) return;

    EnsureBufferSize(device, w, h);

    uint32_t totalPixels = w * h;

    // Restart progressive render when invalidated
    if (m_dirty) {
        std::fill(m_pixelBuffer.begin(), m_pixelBuffer.end(), 0xFF000000);

        m_pixelOrder.resize(totalPixels);
        for (uint32_t i = 0; i < totalPixels; ++i)
            m_pixelOrder[i] = i;
        std::shuffle(m_pixelOrder.begin(), m_pixelOrder.end(), m_rng);

        m_nextPixelIdx = 0;
        m_renderComplete = false;
        m_dirty = false;
    }

    // Trace a batch of pixels if not yet complete
    if (!m_renderComplete) {
        // ~10 seconds at 60 fps -> 600 frames total
        uint32_t pixelsPerFrame = std::max(1u, totalPixels / 600u);
        uint32_t endIdx = std::min(m_nextPixelIdx + pixelsPerFrame, totalPixels);

        for (uint32_t i = m_nextPixelIdx; i < endIdx; ++i) {
            uint32_t pixelIndex = m_pixelOrder[i];
            uint32_t px = pixelIndex % w;
            uint32_t py = pixelIndex / w;

            Vec3 color = ShadePixel(px, py, w, h, camera, scene);

            uint8_t r = (uint8_t)(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
            uint8_t g = (uint8_t)(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
            uint8_t b = (uint8_t)(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
            m_pixelBuffer[py * w + px] = (255u << 24) | (b << 16) | (g << 8) | r;
        }

        m_nextPixelIdx = endIdx;
        if (m_nextPixelIdx >= totalPixels)
            m_renderComplete = true;
    }

    UploadAndDraw(device, target);
}
