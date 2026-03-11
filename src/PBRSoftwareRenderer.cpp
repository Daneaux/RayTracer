#include "renderer/PBRSoftwareRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <cmath>
#include <algorithm>
#include "Object.h"
#include <cassert>
#include "renderer/utils.h"

bool PBRSoftwareRenderer::Initialize(DXDevice& device, uint32_t width, uint32_t height)
{
    maxDepth = 3;
    m_bufWidth = width;
    m_bufHeight = height;
    m_pixelBuffer.resize(width * height, 0xFF000000);

    m_quad = std::make_unique<FullscreenQuad>();
    return m_quad->Initialize(device, width, height);
}

void PBRSoftwareRenderer::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    // Buffer will be resized in Render if target dimensions differ
    (void)device;
    (void)width;
    (void)height;
}

void PBRSoftwareRenderer::Render(
    DXDevice& device, Scene& scene,
    const Camera& camera, SwapChainTarget& target)
{
    uint32_t w = target.GetWidth();
    uint32_t h = target.GetHeight();

    if (w == 0 || h == 0) return;

    if (w != m_bufWidth || h != m_bufHeight) {
        m_bufWidth = w;
        m_bufHeight = h;
        m_pixelBuffer.resize(w * h);
        m_quad->Resize(device, w, h);
    }

    Material air;
    air.baseColor = Vec3(0, 0, 0);
    air.ior = 1.0;
    // todo: other params and move to initializer

    for (uint32_t py = 0; py < h; ++py) {
        for (uint32_t px = 0; px < w; ++px) {
            // ============================================================
            // INNER LOOP: This runs once per pixel (px, py).
            // To paint a pixel, write an ABGR uint32_t to:
            //     m_pixelBuffer[py * w + px]
            // Format: (A << 24) | (B << 16) | (G << 8) | R
            //
            // --- SAMPLE: paint a pixel directly ---
            // Example 1: Solid red pixel
            //   m_pixelBuffer[py * w + px] = 0xFF0000FF; // A=FF B=00 G=00 R=FF
            //
            // Example 2: XOR checkerboard pattern
            //   uint8_t xor_val = (uint8_t)((px ^ py) & 0xFF);
            //   m_pixelBuffer[py * w + px] = (255u << 24) | (xor_val << 16) | (xor_val << 8) | xor_val;
            //
            // Example 3: Animated concentric circles (using a frame counter)
            //   float cx = (float)px - w * 0.5f;
            //   float cy = (float)py - h * 0.5f;
            //   float dist = std::sqrt(cx * cx + cy * cy);
            //   uint8_t ring = (uint8_t)((int)(dist * 0.1f) & 1 ? 255 : 0);
            //   m_pixelBuffer[py * w + px] = (255u << 24) | (ring << 8); // green rings
            // ============================================================

            float ndcX = (2.0f * (px + 0.5f) / w) - 1.0f;
            float ndcY = 1.0f - (2.0f * (py + 0.5f) / h);

            Vec3 origin, direction;
            camera.GenerateRay(ndcX, ndcY, origin, direction);

            float screenU = (float)px / (float)w;
            float screenV = (float)py / (float)h;

            Vec3 color = TraceRay(Ray3(origin, direction), scene, air, maxDepth);

            uint8_t r = (uint8_t)(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
            uint8_t g = (uint8_t)(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
            uint8_t b = (uint8_t)(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
            m_pixelBuffer[py * w + px] = (255u << 24) | (b << 16) | (g << 8) | r;
        }
    }

    m_quad->Upload(device, m_pixelBuffer.data(), w, h);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    target.Bind(device);
    target.Clear(device, clearColor);
    m_quad->Draw(device);
}

Vec3 PBRSoftwareRenderer::TraceRay(
    Ray3 ray,
    Scene& scene,
    Material& currentMaterial,
    int currentDepth)
//Vec3 PBRSoftwareRenderer::PBRTraceWithBeer(Ray3 ray, Scene& scene) 
{
    Vec3 accumulated_radiance(0, 0, 0);
    Vec3 throughput(1, 1, 1); // Tracks light attenuation per bounce

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce)
    {
        Vec3 outHit, normalA;
        WorldObject* obj = FindClosestHit(ray, scene, outHit, normalA);

        Intersection hit;
        hit.occurred = obj != nullptr;
        if (!hit.occurred) {
            accumulated_radiance += throughput * scene.GetAmbientColor();
            break;
        }
        hit.material = obj->GetMaterial();
        hit.normal = normalA;
        hit.point = outHit;
        hit.distance = (ray.origin - outHit).Length();

        Material& m = hit.material;
        accumulated_radiance += throughput * m.emissive;

        // --- 1. HANDLE TRANSMISSION & BEER'S LAW ---
        float rf = randomGen.GetNext();
        if (m.transmission > 0.0f && rf < m.transmission) {
            bool entering = Vec3::Dot(ray.direction, hit.normal) < 0;
            Vec3 outward_normal = entering ? hit.normal : -hit.normal;

            float n1, n2;
            if (entering)
            {
                n1 = 1.0f;
                n2 = m.ior;
            }
            else
            {
                n1 = m.ior;
                n2 = 1.0f;
            }

            Vec3 refracted_dir;
            bool isRefraction = RefractRay(ray.direction, outward_normal, n1, n2, refracted_dir);
            if (isRefraction) {
                // If exiting the material, apply Beer's Law absorption
                if (!entering) {
                    Vec3 sigma_a = -Vec3::log(m.baseColor + 0.001f) / 1.0f; // Absorption coeff
                    throughput *= Vec3::exp(-sigma_a * hit.distance);
                }
                ray = Ray3(hit.point + refracted_dir * 0.001f, refracted_dir);
                continue; // Skip NEE/Diffuse for pure transmission
            }
        }

        // --- 2. NEXT EVENT ESTIMATION (Direct Light Sampling) ---
        // Fire a shadow ray directly to a random light to reduce noise
        std::vector<LightHitDirTuple> lightHitTuples;
        bool isLit = CastShadowRays(ray.origin, outHit, scene, lightHitTuples);
        if (isLit)
        {
            LightHitDirTuple& tuple = lightHitTuples[0]; // todo: for now we're assuming 1 pointlight

            PointLight& pl = dynamic_cast<PointLight&>(tuple.light);
            LightSample light = sample_point_light(outHit, pl);
            float bsdf_pdf = m.calculate_pdf(hit.normal, -ray.direction, tuple.surfaceToLightNormalized);
            float mis_weight = power_heuristic(light.pdf, bsdf_pdf);
            accumulated_radiance += throughput * m.baseColor * light.radiance * mis_weight;
        }

        // --- 3. INDIRECT BOUNCE (Stochastic Sampling) ---
        // Choose between Diffuse or Specular based on metallic/roughness
        Vec3 next_dir = sample_pbr_direction(m, hit.normal, -ray.direction);
        float pdf = m.calculate_pdf(hit.normal, -ray.direction, next_dir);

        // Rendering Equation: (BRDF * cos) / PDF
        throughput *= (m.baseColor * std::max(0.0f, Vec3::Dot(hit.normal, next_dir))) / pdf;
        ray = Ray3(hit.point + next_dir * 0.001f, next_dir);

        // --- 4. RUSSIAN ROULETTE ---
        // Randomly terminate paths that have low throughput to save performance
        float p = std::max(throughput.x, std::max(throughput.y, throughput.z));
        if (bounce > 3) {
            rf = randomGen.GetNext();
            if (rf > p) break;
            throughput /= p;
        }
    }
    return accumulated_radiance;
}

WorldObject* PBRSoftwareRenderer::FindClosestHit(
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
bool PBRSoftwareRenderer::CastShadowRays(Vec3& origin, const Vec3& hitPoint, const Scene& scene, std::vector<LightHitDirTuple>& lightHitTuples)
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
        if (occluder != nullptr) {
            // Only count as occluder if it's between the hit point and the light
            float occluderDist = (tempHit - hitPoint).Length();
            occluded = (occluderDist < distToLight - 0.001f);
        }
        if (!occluded) {
            isLit = true;
            Vec3 surfaceToLight = toLight.Normalized();
            lightHitTuples.push_back(LightHitDirTuple{ *light, surfaceToLight, distToLight });
        }
    }

    return isLit;
}




