#pragma once

#include "renderer/IRenderer.h"
#include "renderer/FullscreenQuad.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>
#include <cstdint>
#include "Object.h"
#include "Scene.h"
#include "Camera.h"
#include "Lights.h"


class SoftwareRenderer : public IRenderer {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;
    void OnResize(DXDevice& device, uint32_t width, uint32_t height) override;
    void Render(DXDevice& device, const Scene& scene,
                const Camera& camera, SwapChainTarget& target) override;

private:
    Vec3 TraceRay(const Vec3& origin, const Vec3& direction, const Scene& scene,
                  float screenU, float screenV) const;

    Vec3 TraceRay_orig(const Vec3& origin, const Vec3& direction, const Scene& scene,
        float screenU, float screenV) const;

    bool IntersectSphere(
        const Vec3& origin, 
        const Vec3& dir,
        const SphereObject& sphere, 
        float& t) const;

    Vec3 ComputePhongLighting(const Vec3& hitPoint, const Vec3& normal,
                              const Vec3& viewDir, const SphereObject& sphere,
                              const PointLight& light, const Vec3& ambient) const;

    std::vector<uint32_t>           m_pixelBuffer;
    uint32_t                        m_bufWidth = 0;
    uint32_t                        m_bufHeight = 0;
    std::unique_ptr<FullscreenQuad> m_quad;
};
