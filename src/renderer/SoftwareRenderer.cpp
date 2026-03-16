#include "renderer/SoftwareRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <cmath>
#include <algorithm>
#include "Object.h"
#include <cassert>
#include "renderer/utils.h"
#include <string>


bool SoftwareRenderer::Initialize(DXDevice& device, uint32_t width, uint32_t height) {
    if (!SoftwareRendererBase::Initialize(device, width, height))
        return false;
    maxDepth = 8;
    return true;
}

Vec3 SoftwareRenderer::ShadePixel(uint32_t px, uint32_t py, uint32_t w, uint32_t h,
    const Camera& camera, Scene& scene) {
    Material air;
    air.baseColor = Vec3(0, 0, 0);
    air.ior = 1.0;

    float ndcX = (2.0f * (px + 0.5f) / w) - 1.0f;
    float ndcY = 1.0f - (2.0f * (py + 0.5f) / h);

    Vec3 origin, direction;
    camera.GenerateRay(ndcX, ndcY, origin, direction);

    return TraceRay(Ray3(origin, direction), scene, air, maxDepth);
}

Vec3 SoftwareRenderer::TraceRay(
    Ray3 ray,
    Scene& scene,
    Material& currentMaterial,
    int currentDepth)
{
    if (currentDepth <= 0) return Vec3(0.1f, 0.01f, 0.01f);

    Vec3 outHit, normalA, outB, normalB;

    //
    // -- Find closest hit object
    //
    WorldObject* obj = FindClosestHit(ray, scene, outHit, normalA);
    if (obj == nullptr) {
        return scene.GetAmbientColor();
    }

    // am I inside an object?  
    // todo: temp hack ... either 1) create one sided objects or 2) add a flag for objects that have an interior
    bool isInside = (Vec3::Dot(ray.direction, normalA) > 0.0f) && !(obj->IsQuad());

    Material mat = obj->GetMaterial();
    if (obj->IsQuad()) {
        QuadObject* quad = (QuadObject*)obj;
        if (quad->useCheckeredPattern) {
            Vec3 checkerColor = quad->GetColorAtPoint(outHit);
            mat.baseColor = checkerColor;
        }
    }

    //
    // -- Cast shadow rays to determine lighting contribution at hit point
    //
    std::vector<LightHitDirTuple> lightHitTuples;
    //ShadingComponents shadeComponents;
    bool isLit = CastShadowRays(obj, ray.origin, outHit, scene, lightHitTuples);
    Vec3 diffuseLIghting(0.0f, 0.0f, 0.0f);
    if (isLit && !isInside) // doesn't make sense to diffuse light inside of a sphere for example
    {
        Vec3 viewDir = -ray.direction.Normalized();
        for (LightHitDirTuple& tuple : lightHitTuples)
        {
            //shadeComponents = BlinnPhongSeparated(tuple, normalA, viewDir, mat);
            diffuseLIghting += BlinnPhong(tuple, normalA, viewDir, mat);
        }
    }

    //
    // -- Cast more rays: reflection, refraction, etc.
    //
    float n1, n2;
    n1 = currentMaterial.ior;
    n2 = mat.ior;

    bool doRR = true;
    float alpha = mat.transmission;
    Vec3 colorKr = Vec3(0, 0, 0);
    Vec3 colorKt = Vec3(0, 0, 0);
    float kr, kt;

    if (isInside)
    {
        // todo: ray length in sphere modulated by material light loss later.
        //assert(&currentMaterial == &mat);
        // todo: we're assuming exiting any object goes outside into the air with IOR = 1.0
        FresnelSchlick(ray.direction.Normalized(), -normalA, n1, 1.0f, kr, kt);
        // no reflection (yet) inside a sphere
        assert(alpha > 0.0f); // why would be inside a sphere if alpha is zero?
        if (kt > 0.0f && alpha > 0.0f)
        {
            Vec3 refractRay;
            bool isRefraction = RefractRay(ray.direction, normalA, n1, n2, refractRay);
            if (isRefraction)
            {
                Vec3 rayOrigin = outHit + normalA * 0.001f; // move new origin outside of sphere slightly
                colorKt = kt * TraceRay(Ray3(rayOrigin, refractRay), scene, mat, currentDepth - 1);
            }
        }
    }
    else
    {
        if (!isLit)
        {
            int x = 4;
        }

        FresnelSchlick(ray.direction.Normalized(), normalA, n1, n2, kr, kt);
        Vec3 reflectedRay = ReflectRay(ray.direction, normalA);
        Vec3 rayOrigin = outHit + normalA * 0.001f;
        colorKr = kr * TraceRay(Ray3(rayOrigin, reflectedRay), scene, mat, currentDepth - 1);

        if (kt > 0.0f && alpha > 0.0f)
        {
            Vec3 refractRay;
            bool isRefraction = RefractRay(ray.direction, normalA, n1, n2, refractRay);
            if (isRefraction)
            {
                Vec3 rayOrigin = outHit - normalA * 0.001f;
                colorKt = kt * TraceRay(Ray3(rayOrigin, refractRay), scene, mat, currentDepth - 1);
            }
        }
    }


    bool version1 = false;
    Vec3 finalColor = { 0, 0, 0 };
    if (version1)
    {
        if (isLit)
            //finalColor += shadeComponents.specular;

        if (alpha > 0.0f)
            finalColor += colorKt * alpha;

        if (alpha < 1.0f && isLit)
            //finalColor += shadeComponents.diffuse * (1.0f - alpha);

        finalColor += colorKr;
    }
    else {
        float specular = 1.0f - mat.roughness;

        if (specular > 0.0f && mat.transmission > 0.0f)
        {
            // reflect and refract
            finalColor += colorKt * alpha;
            finalColor += colorKr * specular;
        }
        else if (specular > 0.0f)
        {
            // reflect only
            finalColor += colorKr * specular;// +diffuseLIghting / specular;
        }
        else
        {
            finalColor += colorKr + diffuseLIghting + mat.emissive;
        }

    }

    bool clamp = false;
    if (clamp)
    {
        return {
            std::min(finalColor.x, 1.0f),
            std::min(finalColor.y, 1.0f),
            std::min(finalColor.z, 1.0f)
        };
    }
    else
    {
        return finalColor;
    }
}

WorldObject * SoftwareRenderer::FindClosestHit(
    const Ray3& ray,
    const Scene& scene,
    Vec3& outHitPoint,
    Vec3& outNormal) const
{
    WorldObject* closestObj = nullptr;
    float closestT = std::numeric_limits<float>::max();
    for (auto* obj : scene.GetObjects()) {
        float t;
        Vec3 hitPointA, normalA, hitPointB, normalB;
        if (obj->IsHitByRay(ray, hitPointA, t, normalA, hitPointB, normalB)) {
            float t2 = (hitPointA - ray.origin).Length();
			assert(std::abs(t - t2) < 0.01f); // sanity check that t matches hit point distance
            if (t < closestT) {
                closestT = t;
                closestObj = obj;
                outHitPoint = hitPointA;
                outNormal = normalA;
            }
        }
    }
    return closestObj;
}

//  assert(dynamic_cast<QuadObject*>(objectToSkip) != nullptr);

bool SoftwareRenderer::IsAnyHit(
    WorldObject* objectToSkip,
    const Ray3& ray,
    const Scene& scene,
    Vec3 &occluderHitPoint,
    WorldObject** occluder)
{
    for (auto* obj : scene.GetObjects()) {
        if (obj == objectToSkip)        
            continue;

        float t;
        Vec3 hitPointA, normalA, hitPointB, normalB;
        if (obj->IsHitByRay(ray, hitPointA, t, normalA, hitPointB, normalB)) {
            assert(t > 0.0f);
            occluderHitPoint = hitPointA;
            *occluder = obj;
            return true;
        }
    }
    return false;
}

// casts rays from hit point to all lights to determine if in shadow and how much light contributes.
// For simplicity, we return a single color multiplier for all lights (1.0 = fully lit, 0.0 = in shadow).
bool SoftwareRenderer::CastShadowRays(WorldObject *lastHitObject, Vec3& origin, const Vec3& hitPoint, const Scene& scene, std::vector<LightHitDirTuple>& lightHitTuples)
{
    bool isLit = false;
    for (Light* light : scene.GetLights()) {
        Vec3 toLight = light->position - hitPoint;
        float distToLight = toLight.Length();
        Vec3 shadowRayDir = toLight; // does this need to be normalized?

        Vec3 shadowOrigin = hitPoint + (shadowRayDir * 0.001f);

        Ray3 shadowRay = Ray3(shadowOrigin, shadowRayDir);

        // Check if shadow ray is occluded by any object
        Vec3 occluderHitPoint;
        WorldObject* occluder = nullptr;
        bool isHit = IsAnyHit(lastHitObject, shadowRay, scene, occluderHitPoint, &occluder);
        bool occluded = false;
        if(isHit) {
            // Only count as occluder if it's between the hit point and the light
            float occluderDist = (occluderHitPoint - shadowOrigin).Length();
            occluded = (occluderDist < distToLight - 0.001f);
		}

        if (!occluded) {
            isLit = true;
            lightHitTuples.push_back(LightHitDirTuple{ *light, toLight.Normalized(), distToLight });
		}
    }

    return isLit;
}
