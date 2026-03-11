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

    maxDepth = 8;
    m_bufWidth = width;
    m_bufHeight = height;
    m_pixelBuffer.resize(width * height, 0xFF000000);

    m_quad = std::make_unique<FullscreenQuad>();
    return m_quad->Initialize(device, width, height);
}

void SoftwareRenderer::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    (void)device;
    (void)width;
    (void)height;
    m_dirty = true;
}

void SoftwareRenderer::Render(DXDevice& device, Scene& scene,
                               const Camera& camera, SwapChainTarget& target) {
    uint32_t w = target.GetWidth();
    uint32_t h = target.GetHeight();

    if (w == 0 || h == 0) return;

    // Handle resize
    if (w != m_bufWidth || h != m_bufHeight) {
        m_bufWidth = w;
        m_bufHeight = h;
        m_pixelBuffer.resize(w * h);
        m_quad->Resize(device, w, h);
        m_dirty = true;
    }

    uint32_t totalPixels = w * h;

    // Restart progressive render when invalidated
    if (m_dirty) {
        std::fill(m_pixelBuffer.begin(), m_pixelBuffer.end(), 0xFF000000); // opaque black

        // Build shuffled pixel order
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
        Material air;
        air.baseColor = Vec3(0, 0, 0);
        air.ior = 1.0;

        // ~10 seconds at 60 fps → 600 frames total
        uint32_t pixelsPerFrame = std::max(1u, totalPixels / 600u);
        uint32_t endIdx = std::min(m_nextPixelIdx + pixelsPerFrame, totalPixels);

        for (uint32_t i = m_nextPixelIdx; i < endIdx; ++i) {
            uint32_t pixelIndex = m_pixelOrder[i];
            uint32_t px = pixelIndex % w;
            uint32_t py = pixelIndex / w;

            float ndcX = (2.0f * (px + 0.5f) / w) - 1.0f;
            float ndcY = 1.0f - (2.0f * (py + 0.5f) / h);

            Vec3 origin, direction;
            camera.GenerateRay(ndcX, ndcY, origin, direction);

            Vec3 color = TraceRay(Ray3(origin, direction), scene, air, maxDepth);

            uint8_t r = (uint8_t)(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
            uint8_t g = (uint8_t)(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
            uint8_t b = (uint8_t)(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
            m_pixelBuffer[py * w + px] = (255u << 24) | (b << 16) | (g << 8) | r;
        }

        m_nextPixelIdx = endIdx;
        if (m_nextPixelIdx >= totalPixels)
            m_renderComplete = true;
    }

    // Always upload and draw so the swap chain stays valid
    m_quad->Upload(device, m_pixelBuffer.data(), w, h);

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    target.Bind(device);
    target.Clear(device, clearColor);
    m_quad->Draw(device);
}

Vec3 SoftwareRenderer::TraceRay(
    Ray3 ray,
    Scene& scene,
    Material& currentMaterial,
    int currentDepth)
{
    if (currentDepth <= 0) return Vec3(0, 0, 0);

    Vec3 outHit, normalA, outB, normalB;

    // 
    // -- Find closest hit object
    // 
    WorldObject* obj = FindClosestHit(ray, scene, outHit, normalA);
    if (obj == nullptr) {
        return scene.GetAmbientColor();
    }

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
    ShadingComponents shadeComponents;
    bool isLit = CastShadowRays(ray.origin, outHit, scene, lightHitTuples);
    if (isLit)
    {
        // do diffuse shading if any, color additive (not averaged)
        // NOTE: this *should* averaged if we cast many rays towards the lights we'd need to average their collective (per light) contribution.
        // for now, however, we're casting one ray towards each light.
        Vec3 viewDir = -ray.direction.Normalized();
        for (LightHitDirTuple& tuple : lightHitTuples)
        {
            //finalColor += BlinnPhongWithLightAttenuation(tuple, normalA, viewDir, mat);
            shadeComponents = BlinnPhongSeparated(tuple, normalA, viewDir, mat);
        }
    }

    // 
    // -- Cast more rays: reflection, refraction, etc.
    // 
    float n1, n2;
    n1 = currentMaterial.ior;
    n2 = mat.ior;

    bool doRR = true;

    Vec3 colorKr = Vec3(0, 0, 0);
    Vec3 colorKt = Vec3(0, 0, 0);

    if (doRR)
    {
        float kr, kt;
        float alpha = mat.transmission;

        FresnelSchlick(ray.direction, normalA, n1, n2, kr, kt);

        Vec3 reflectedRay = ReflectRay(ray.direction, normalA);
        Vec3 rayOrigin = outHit + normalA * 0.001f;
        colorKr = kr * TraceRay(Ray3(rayOrigin, reflectedRay), scene, mat, currentDepth - 1);
        
        if (kt > 0.0f)
        {
            Vec3 refractRay;
            bool isRefraction = RefractRay(ray.direction, normalA, n1, n2, refractRay);
            if (isRefraction)
            {
                Vec3 rayOrigin = outHit - normalA * 0.001f;
                colorKt = kt * TraceRay(Ray3(rayOrigin, refractRay), scene, mat, currentDepth - 1);                
            }
        }

        //finalColor += colorRT;
        //finalColor += (1.0f - alpha) * mat.baseColor;
    }
    else
    {
        //finalColor += scene.GetAmbientColor();
    }


    Vec3 finalColor = { 0, 0, 0 };
    if (isLit)    
        finalColor += shadeComponents.specular;    

    float T = mat.transmission;
    if (T > 0.0f)
        finalColor += colorKt * T;

    if (T < 1.0f && isLit)
        finalColor += shadeComponents.diffuse * (1.0f - T);

    finalColor += colorKr;

    return {
        std::min(finalColor.x, 1.0f),
        std::min(finalColor.y, 1.0f),
        std::min(finalColor.z, 1.0f)
    };
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

// casts rays from hit point to all lights to determine if in shadow and how much light contributes. 
// For simplicity, we return a single color multiplier for all lights (1.0 = fully lit, 0.0 = in shadow).
bool SoftwareRenderer::CastShadowRays(Vec3& origin, const Vec3& hitPoint, const Scene& scene, std::vector<LightHitDirTuple>& lightHitTuples)
{
    bool isLit = false;
    for (Light* light : scene.GetLights()) {
        Vec3 toLight = light->position - hitPoint;
        float distToLight = toLight.Length();
        Vec3 shadowRayDir = toLight; // does this need to be normalized?
        // Check if shadow ray is occluded by any object
        Vec3 tempHit, tempNormal;
        WorldObject* occluder = FindClosestHit(Ray3(hitPoint, shadowRayDir), scene, tempHit, tempNormal); // todo: replace with find any hit. optimized.
        bool occluded = false;
        if(occluder != nullptr) {
            // Only count as occluder if it's between the hit point and the light
            float occluderDist = (tempHit - hitPoint).Length();
            occluded = (occluderDist < distToLight - 0.001f);
		}
        if (!occluded) {
            isLit = true;
            lightHitTuples.push_back(LightHitDirTuple{ *light, toLight.Normalized(), distToLight });
		}
    }

    return isLit;
}




