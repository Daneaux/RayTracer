#pragma once

#include "renderer/SoftwareRendererBase.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>
#include <cstdint>
#include "Object.h"
#include "Lights.h"
#include "Scene.h"

class PointLight;

class SoftwareOverviewRenderer : public SoftwareRendererBase {
public:
    void Render(
        DXDevice& device,
        Scene& scene,
        const Camera& camera,
        SwapChainTarget& target) override;

    void SetObservedCamera(const Camera* cam) { m_observedCamera = cam; }

protected:
    // Not used (Render is overridden for line drawing), but required by base class.
    Vec3 ShadePixel(uint32_t, uint32_t, uint32_t, uint32_t,
                    const Camera&, Scene&) override { return Vec3(0, 0, 0); }

private:
    uint32_t ColorToABGR(const Vec4& color) const;
    void SetPixel(int x, int y, uint32_t color);
    void DrawLine2D(int x0, int y0, int x1, int y1, uint32_t color);
    void DrawLine3D(const Vec3& a, const Vec3& b, const Vec4& color, const Mat4& vp);

    void DrawGrid(const Mat4& vp);
    void DrawSphereWireframe(const SphereObject& sphere, const Mat4& vp);
    void DrawLightIndicator(PointLight& light, const Mat4& vp);
    void DrawFrustum(const Camera& cam, const Mat4& vp);

    const Camera* m_observedCamera = nullptr;
};
