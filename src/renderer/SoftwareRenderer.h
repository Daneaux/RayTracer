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
#include <random>

struct LightHitDirTuple
{
    Light& light;
    Vec3 fromLightToHit;
    float lightDistance;
};

class SoftwareRenderer : public IRenderer {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;
    void OnResize(DXDevice& device, uint32_t width, uint32_t height) override;
    void Render(DXDevice& device, Scene& scene,
                const Camera& camera, SwapChainTarget& target) override;

private:
    Vec3 LambertShade(LightHitDirTuple& tuple, Vec3& normalA, WorldObject* obj);

    Vec3 TraceRay(
        Vec3& origin,
        Vec3& direction,
        Scene& scene,
        float screenU,
        float screenV,
        Material& currentMaterial,
        int currentDepth);

    WorldObject* FindClosestHit(const Vec3& origin, const Vec3& direction, const Scene& scene, Vec3& outHitPoint, Vec3& outNormal) const;

    Vec3 TraceRay_orig(Vec3& origin, Vec3& direction, Scene& scene,
        float screenU, float screenV);

    bool IntersectSphere(
        const Vec3& origin, 
        const Vec3& dir,
        const SphereObject& sphere, 
        float& t) const;

    Vec3 ComputePhongLighting(const Vec3& hitPoint, const Vec3& normal,
                              const Vec3& viewDir, const SphereObject& sphere,
                              const PointLight& light, const Vec3& ambient) const;

    bool CastShadowRays(Vec3& origin, const Vec3& hitPoint, const Scene& scene, std::vector<LightHitDirTuple>& lightHitTuples);

    std::vector<uint32_t>           m_pixelBuffer;
    uint32_t                        m_bufWidth = 0;
    uint32_t                        m_bufHeight = 0;
    std::unique_ptr<FullscreenQuad> m_quad;

    int                             maxDepth;

    std::random_device              rd;
    std::mt19937                    randomGenerator;
    std::uniform_real_distribution<> randomDistribution;
};
