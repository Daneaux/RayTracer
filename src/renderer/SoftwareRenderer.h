#pragma once

#include "renderer/SoftwareRendererBase.h"
#include "math/MathTypes.h"
#include <vector>
#include <cstdint>
#include "Object.h"
#include "Scene.h"
#include "Camera.h"
#include "Lights.h"
#include "renderer/utils.h"

class SoftwareRenderer : public SoftwareRendererBase {
public:
    bool Initialize(DXDevice& device, uint32_t width, uint32_t height) override;

protected:
    Vec3 ShadePixel(uint32_t px, uint32_t py, uint32_t w, uint32_t h,
                    const Camera& camera, Scene& scene) override;

private:
    Vec3 TraceRay(
        Ray3 ray,
        Scene& scene,
        Material& currentMaterial,
        int currentDepth);

    WorldObject* FindClosestHit(const Ray3& ray, const Scene& scene, Vec3& outHitPoint, Vec3& outNormal) const;

    Vec3 TraceRay_orig(
        Vec3& origin,
        Vec3& direction,
        Scene& scene,
        float screenU,
        float screenV);

    bool IntersectSphere(
        const Vec3& origin,
        const Vec3& dir,
        const SphereObject& sphere,
        float& t) const;

    Vec3 ComputePhongLighting(const Vec3& hitPoint, const Vec3& normal,
                              const Vec3& viewDir, const SphereObject& sphere,
                              const PointLight& light, const Vec3& ambient) const;

    bool CastShadowRays(Vec3& origin, const Vec3& hitPoint, const Scene& scene, std::vector<LightHitDirTuple>& lightHitTuples);

    int maxDepth = 8;
};
