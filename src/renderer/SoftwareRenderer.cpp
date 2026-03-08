#include "renderer/SoftwareRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <cmath>
#include <algorithm>
#include "Object.h"
#include <cassert>


static Vec3 GetAverageColor(const std::vector<Vec3>& colors)
{
    if (colors.empty()) return Vec3(0, 0, 0);
    Vec3 sum(0, 0, 0);
    for (const Vec3& c : colors) {
        sum += c;
    }
    float inv = 1.0f / static_cast<float>(colors.size());
    return sum * inv;
}

// Function to calculate reflection
Vec3 ReflectRay(const Vec3& incident, const Vec3& normal) {
    // Formula: R = I - 2 * dot(I, N) * N
    // Ensure normal is normalized
    Vec3 n = normal.Normalized();
    return incident - n * (2.0f * Vec3::Dot(incident, n));
}

// Function to calculate refracted ray direction
// incident: Normalized incoming ray direction
// normal: Normalized surface normal
// n_outside: Refractive index of outside medium (e.g., air=1.0)
// n_inside: Refractive index of inside medium (e.g., glass=1.5)
// refractedDir: Output normalized refracted ray direction
// Returns: true if refraction occurred, false if Total Internal Reflection (TIR)
bool RefractRay(
    const Vec3& incident,
    const Vec3& normal,
    double n_outside,
    double n_inside,
    Vec3& refractedDir) {

    double n1 = n_outside;
    double n2 = n_inside;
    Vec3 n = normal;

    double cosI = -Vec3::Dot(n, incident);

    // Check if we are entering or leaving the medium
    if (cosI < 0) {
        // Leaving the medium: invert normal and swap indices
        cosI = -Vec3::Dot(n, incident); // cosI is now positive
        n = -normal;
        std::swap(n1, n2);
    }

    double eta = n1 / n2;
    double sinT2 = eta * eta * (1.0 - cosI * cosI);

    // Total Internal Reflection (TIR)
    if (sinT2 > 1.0) return false;

    double cosT = std::sqrt(1.0 - sinT2);

    // Vector form of Snell's Law
    refractedDir = (incident * eta + n * (eta * cosI - cosT)).Normalized();
    return true;
}

float schlick_reflectance(float cosine, float ref_idx)
{
    // Calculate R0 (the reflectance at normal incidence)
    float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0; // R0 = ((n1 - n2) / (n1 + n2))^2

    // Apply Schlick's approximation formula
    return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5);
}

// At this angle, outReflectance means:
// 0.0 = fully refracted (perfectly transparent), 1.0 = fully reflected (perfect mirror).
// In practice, the actual reflected color would be the incoming ray color multiplied by outReflectance, 
// and the refracted color would be the incoming ray color multiplied by (1.0f - outReflectance).
void  FresnelSchlick(
    Vec3& incomingRayNormalized,
    Vec3& normalNormalized,
    float n1, float n2,
    float& outReflectance,
    float& outTransmittance)
{
    float refractionRatio = n1 / n2;
    // 1. Calculate the cosine of the angle of incidence
    float cos_theta = std::fmin(Vec3::Dot(-incomingRayNormalized, normalNormalized), 1.0);

    // 2. Determine the ratio (reflectance)
    outReflectance = schlick_reflectance(cos_theta, refractionRatio);
    outTransmittance = 1.0f - outReflectance;
}

bool SoftwareRenderer::Initialize(DXDevice& device, uint32_t width, uint32_t height) {

    maxDepth = 5;
    m_bufWidth = width;
    m_bufHeight = height;
    m_pixelBuffer.resize(width * height, 0xFF000000);

    randomGenerator = std::mt19937(rd());
    randomDistribution = std::uniform_real_distribution<>(0.0, 1.0);

    m_quad = std::make_unique<FullscreenQuad>();
    return m_quad->Initialize(device, width, height);
}

void SoftwareRenderer::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    // Buffer will be resized in Render if target dimensions differ
    (void)device;
    (void)width;
    (void)height;
}

void SoftwareRenderer::Render(DXDevice& device, Scene& scene,
                               const Camera& camera, SwapChainTarget& target) {
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
    air.diffuseColor = Vec3(0, 0, 0);
    air.refractionIndex = 1.0;
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

            Vec3 color = TraceRay(origin, direction, scene, screenU, screenV, air, maxDepth);            

            uint8_t r = (uint8_t)(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
            uint8_t g = (uint8_t)(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
            uint8_t b = (uint8_t)(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
            m_pixelBuffer[py * w + px] = (255u << 24) | (b << 16) | (g << 8) | r;
        }
    }

    m_quad->Upload(device, m_pixelBuffer.data(), w, h);

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    target.Bind(device);
    target.Clear(device, clearColor);
    m_quad->Draw(device);
}

Vec3 SoftwareRenderer::LambertShade(LightHitDirTuple& tuple, Vec3& normalA, WorldObject* obj)
{
    Light& light = tuple.light;
    Vec3& hitDir = tuple.fromLightToHit.Normalized();
    float diffuseFactor = Vec3::Dot(normalA, hitDir);
    diffuseFactor = std::max(0.0f, diffuseFactor);
    float attenuation = 1.0f / (tuple.lightDistance * tuple.lightDistance + 1.0f);

    Vec3 color = attenuation * diffuseFactor * light.color * light.intensity * obj->GetMaterial().diffuseColor;
    return color;
}

Vec3 SoftwareRenderer::TraceRay(
    Vec3& origin,
    Vec3& direction,
    Scene& scene,
    float screenU,
    float screenV,
    Material& currentMaterial,
    int currentDepth)
{
    if (currentDepth <= 0) return Vec3(0, 0, 0);

    Vec3 outHit, normalA, outB, normalB;


    // 
    // 1. Find closest hit object
    // 
    WorldObject* obj = FindClosestHit(origin, direction, scene, outHit, normalA);
    if (obj == nullptr) {
        // No hit: return background color (gradient based on screen coords for now)
        // later: use scene background.
        return Vec3(screenU, screenV, 0.5f);
    }

    //
    // 2. initialize color with ambient color
    //

    Vec3 finalColor = scene.GetAmbientColor();

    //
    // 3. Cast shadow rays to determine lighting contribution at hit point
    //
    std::vector<LightHitDirTuple> lightHitTuples;
    bool isLit = CastShadowRays(origin, outHit, scene, lightHitTuples);
    if (isLit)
    {
        // do diffuse shading if any, color additive (not averaged)
        // NOTE: this *should* averaged if we cast many rays towards the lights we'd need to average their collective (per light) contribution.
        // for now, however, we're casting one ray towards each light.
        for (LightHitDirTuple& tuple : lightHitTuples)
        {
            finalColor += LambertShade(tuple, normalA, obj);
        }
    }

    // 
    // 4. Cast more rays: reflection, refraction, etc.
    // 
    float n1, n2;
    n1 = currentMaterial.refractionIndex;
    Material mat = obj->GetMaterial();
    n2 = mat.refractionIndex;

    bool doRR = false;

    if (doRR)
    {
        float reflectance, transmittance;
        FresnelSchlick(direction, normalA, n1, n2, reflectance, transmittance);
        if (randomDistribution(randomGenerator) < reflectance) {
            // Cast ONLY a reflection ray
            Vec3 reflectedRay = ReflectRay(direction, normalA);
            finalColor += TraceRay(outHit, reflectedRay, scene, screenU, screenV, mat, currentDepth - 1);
        }
        else {
            // Cast ONLY a refraction ray
            Vec3 refractRay;
            bool isRefraction = RefractRay(direction, normalA, n1, n2, refractRay);
            if (isRefraction)
            {
                finalColor += TraceRay(outHit, refractRay, scene, screenU, screenV, mat, currentDepth - 1);
            }
        }
    }

    //Vec3 averagedColor = GetAverageColor(colors);
    //finalColor += averagedColor;
    return finalColor;
}

WorldObject * SoftwareRenderer::FindClosestHit(
    const Vec3& origin, 
    const Vec3& direction, 
    const Scene& scene,
    Vec3& outHitPoint, 
    Vec3& outNormal) const
{
    WorldObject* closestObj = nullptr;
    Ray3 ray(origin, direction);
    float closestT = std::numeric_limits<float>::max();
    for (auto* obj : scene.GetObjects()) {
        float t;
        Vec3 hitPointA, normalA, hitPointB, normalB;
        if (obj->IsHitByRay(ray, hitPointA, t, normalA, hitPointB, normalB)) {
            float t2 = (hitPointA - origin).Length();
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

Vec3 SoftwareRenderer::ComputePhongLighting(
    const Vec3& hitPoint, 
    const Vec3& normal,                                             
    const Vec3& viewDir, 
    const SphereObject& sphere,                                            
    const PointLight& light,
    const Vec3& ambient) const 
{
	Material mat = sphere.GetMaterial();
    Vec3 L = (light.position - hitPoint).Normalized();
    Vec3 H = (L + viewDir).Normalized();

    float diff = std::max(Vec3::Dot(normal, L), 0.0f);
    float spec = std::pow(std::max(Vec3::Dot(normal, H), 0.0f), mat.specularReflection);

    Vec3 ambientTerm = ambient * mat.diffuseColor;
    Vec3 diffuseTerm = mat.diffuseColor * light.color * (diff * light.intensity);
    Vec3 specularTerm = light.color * (spec * light.intensity);

    return {
        ambientTerm.x + diffuseTerm.x + specularTerm.x,
        ambientTerm.y + diffuseTerm.y + specularTerm.y,
        ambientTerm.z + diffuseTerm.z + specularTerm.z
    };
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
        WorldObject* occluder = FindClosestHit(hitPoint, shadowRayDir, scene, tempHit, tempNormal); // todo: replace with find any hit. optimized.
        bool occluded = false;
        if(occluder != nullptr) {
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




