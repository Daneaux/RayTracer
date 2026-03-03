#pragma once

#include "renderer/IRenderer.h"
#include "renderer/FullscreenQuad.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>
#include <cstdint>

class SoftwareOverviewRenderer : public IRenderer {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;
    void OnResize(DXDevice& device, uint32_t width, uint32_t height) override;
    void Render(DXDevice& device, const Scene& scene,
                const Camera& camera, SwapChainTarget& target) override;

    void SetObservedCamera(const Camera* cam) { m_observedCamera = cam; }

private:
    uint32_t ColorToABGR(const Vec4& color) const;
    void SetPixel(int x, int y, uint32_t color);
    void DrawLine2D(int x0, int y0, int x1, int y1, uint32_t color);
    void DrawLine3D(const Vec3& a, const Vec3& b, const Vec4& color, const Mat4& vp);

    void DrawGrid(const Mat4& vp);
    void DrawSphereWireframe(const struct Sphere& sphere, const Mat4& vp);
    void DrawLightIndicator(const struct PointLight& light, const Mat4& vp);
    void DrawFrustum(const Camera& cam, const Mat4& vp);

    const Camera* m_observedCamera = nullptr;

    std::vector<uint32_t> m_pixelBuffer;
    uint32_t m_bufWidth = 0;
    uint32_t m_bufHeight = 0;
    std::unique_ptr<FullscreenQuad> m_quad;
};
