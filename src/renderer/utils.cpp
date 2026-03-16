#include "Camera.h"
#include "Object.h"
#include "renderer/utils.h"
#include "Scene.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>

template <typename T>
constexpr T lerp(T a, T b, T t) {
    if (t == 0) return a;
    if (t == 1) return b;
    return std::fma(t, b - a, a);
}

Vec3 LambertShadingModel(LightHitDirTuple& tuple, Vec3& normalA, WorldObject* obj)
{
    Light& light = tuple.light;
    Vec3& hitDir = tuple.surfaceToLightNormalized;
    float diffuseFactor = Vec3::Dot(normalA, hitDir);
    diffuseFactor = std::max(0.0f, diffuseFactor);
    float attenuation = 1.0f / (tuple.lightDistance * tuple.lightDistance + 1.0f);

    Vec3 color = attenuation * diffuseFactor * light.color * light.intensity * obj->GetMaterial().baseColor;
    return color;
}

Vec3 BlinnPhongWithLightAttenuation(LightHitDirTuple& tuple, Vec3& hitNormal, Vec3& viewDir, Material& mat)
{
    Light& light = tuple.light;
    Vec3& hitDir = tuple.surfaceToLightNormalized;
    float diffuseFactor = Vec3::Dot(hitNormal, hitDir);
    diffuseFactor = std::max(0.0f, diffuseFactor);
    float attenuation = 1.0f / (tuple.lightDistance * tuple.lightDistance + 1.0f);

    Vec3 halfVec = (hitDir + viewDir).Normalized();
    float specularPower = std::pow(2.0f, 10.0f * (1.0f - mat.roughness));
    float specularFactor = std::pow(std::max(Vec3::Dot(hitNormal, halfVec), 0.0f), specularPower);
    specularFactor = std::max(0.0f, specularFactor);

    Vec3 color = attenuation * light.intensity * (
        diffuseFactor * light.color * mat.baseColor +
        specularFactor * light.color);
    return color;
}

Vec3 BlinnPhong(LightHitDirTuple& tuple, Vec3& hitNormal, Vec3& viewDir, Material& mat)
{

    Light& light = tuple.light;
    Vec3& hitDir = tuple.surfaceToLightNormalized;
    float diffuseFactor = Vec3::Dot(hitNormal, hitDir);
    diffuseFactor = std::max(0.0f, diffuseFactor);

    Vec3 halfVec = (hitDir + viewDir).Normalized();
    float specularPower = std::pow(2.0f, 10.0f * (1.0f - mat.roughness));
    float specularFactor = std::pow(std::max(Vec3::Dot(hitNormal, halfVec), 0.0f), specularPower);
    specularFactor = std::max(0.0f, specularFactor);

    float specularStrength = 1.0f - mat.roughness;
    Vec3 color = light.intensity * (diffuseFactor * light.color * mat.baseColor + specularStrength * specularFactor * light.color);
    return color;
}


ShadingComponents BlinnPhongSeparated(LightHitDirTuple& tuple, Vec3& hitNormal, Vec3& viewDir, Material& mat)
{
    Light& light = tuple.light;
    Vec3& L = tuple.surfaceToLightNormalized;

    // 1. Attenuation
    float partAtt = 0.01f * tuple.lightDistance;
    float attenuation = 1.0f / (1.0f + partAtt + partAtt * tuple.lightDistance);
    float energy = attenuation * light.intensity;

    // 2. Diffuse Component (Lambert)
    float diffuseFactor = std::max(0.0f, Vec3::Dot(hitNormal, L));
    Vec3 diffuseResult = (light.color * mat.baseColor) * (diffuseFactor * energy);

    // 3. Specular Component (Blinn-Phong)
    Vec3 halfVec = (L + viewDir).Normalized();
    float specularPower = std::pow(2.0f, 10.0f * (1.0f - mat.roughness));
    float specularFactor = std::pow(std::max(Vec3::Dot(hitNormal, halfVec), 0.0f), specularPower);
    Vec3 specularResult = light.color * (specularFactor * energy);

    return ShadingComponents { diffuseResult, specularResult };
}


Vec3 offbrandPhong(
    const Vec3& hitPoint,
    const Vec3& normal,
    const Vec3& viewDir,
    const SphereObject& sphere,
    const PointLight& light,
    const Vec3& ambient)
{
    Material mat = sphere.GetMaterial();
    Vec3 L = (light.position - hitPoint).Normalized();
    Vec3 H = (L + viewDir).Normalized();

    float diff = std::max(Vec3::Dot(normal, L), 0.0f);
    float specularPower = std::pow(2.0f, 10.0f * (1.0f - mat.roughness));
    float spec = std::pow(std::max(Vec3::Dot(normal, H), 0.0f), specularPower);

    Vec3 ambientTerm = ambient * mat.baseColor;
    Vec3 diffuseTerm = mat.baseColor * light.color * (diff * light.intensity);
    Vec3 specularTerm = light.color * (spec * light.intensity);

    return ambientTerm + diffuseTerm + specularTerm;
}

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
    const Vec3& incident_,
    const Vec3& normal,
    float n_outside,
    float n_inside,
    Vec3& refractedDir)
{
    Vec3 incident = incident_.Normalized();
    float n1 = n_outside;
    float n2 = n_inside;
    Vec3 n = normal;

    float cosI = -Vec3::Dot(n, incident);

    // Check if we are entering or leaving the medium
    if (cosI < 0) { // We are hitting the surface from the INSIDE
        n = -normal;   // Flip normal to point inward
        cosI = -cosI;  // Now cosI is positive relative to the flipped normal
        std::swap(n1, n2);
    }

    float eta = n1 / n2;
    float sinT2 = eta * eta * (1.0 - cosI * cosI);

    // Total Internal Reflection (TIR)
    if (sinT2 > 1.0) return false;

    float cosT = std::sqrt(1.0 - sinT2);

    // Vector form of Snell's Law
    refractedDir = (incident * eta + n * (eta * cosI - cosT)).Normalized();
    return true;
}

// v3 ?
void FresnelSchlick(const Vec3& incident, const Vec3& normal, float n1, float n2, float& R, float& T)
{
    float cosTheta = Vec3::Dot(-incident, normal);
    float r0 = (n1 - n2) / (n1 + n2);
    r0 = r0 * r0;

    // Schlick's approximation
    R = r0 + (1.0f - r0) * std::pow(1.0f - cosTheta, 5.0f);

    // Total internal reflection check
    if (n1 > n2)
    {
        float sinT2 = (n1 / n2) * std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        if (sinT2 > 1.0f)
        {
            R = 1.0f;
            T = 0.0f;
            return;
        }
    }
    T = 1.0f - R;
}

// At this angle, outReflectance means:
// 0.0 = fully refracted (perfectly transparent), 1.0 = fully reflected (perfect mirror).
// In practice, the actual reflected color would be the incoming ray color multiplied by outReflectance, 
// and the refracted color would be the incoming ray color multiplied by (1.0f - outReflectance).
void  FresnelSchlick_v1(
    Vec3& incomingRayNormalized,
    Vec3& normalNormalized,
    float n1, float n2,
    float& outReflectance,
    float& outTransmittance)
{
    // 1. Calculate the cosine of the angle of incidence
    float cos_theta = std::fmin(Vec3::Dot(-incomingRayNormalized, normalNormalized), 1.0);

    // 2. Determine the ratio (reflectance)

    // R0 = ((n1 - n2) / (n1 + n2))^2
    float r0 = (n1 - n2) / (n1 + n2);
    r0 = r0 * r0;

    outReflectance = r0 + (1.0f - r0) * std::pow((1.0f - cos_theta), 5);
    outTransmittance = 1.0f - outReflectance;
}

void FresnelSchlick_v2(
    const Vec3& incomingRay,
    const Vec3& normal,
    float n1, float n2,
    float& outReflectance,
    float& outTransmittance)
{
    float eta = n1 / n2;
    float cos_theta_i = std::fmin(Vec3::Dot(-incomingRay, normal), 1.0f);

    // Check for Total Internal Reflection (TIR)
    // Using Snell's Law: sin^2(theta_t) = eta^2 * (1 - cos^2(theta_i))
    float sin2_theta_t = eta * eta * (1.0f - cos_theta_i * cos_theta_i);

    if (sin2_theta_t > 1.0f) {
        // TIR occurred: 100% reflection, 0% refraction
        outReflectance = 1.0f;
        outTransmittance = 0.0f;
        return;
    }

    // For Schlick's approx in dense-to-thin transitions, use cos(theta_t)
    float cos_theta = (n1 > n2) ? std::sqrt(1.0f - sin2_theta_t) : cos_theta_i;

    // Calculate Schlick's Reflectance
    float r0 = (n1 - n2) / (n1 + n2);
    r0 = r0 * r0;
    outReflectance = r0 + (1.0f - r0) * std::pow(1.0f - cos_theta, 5.0f);
    outTransmittance = 1.0f - outReflectance;
}


Vec3 align_to_normal(const Vec3& local_dir, const Vec3& normal) {
    // 1. We start with our "Z" axis, which is the surface normal (W)
    Vec3 w = normal.Normalized();

    // 2. We need a helper vector to start the cross-product. 
    // We pick (1,0,0) unless the normal is too close to it, then we use (0,1,0).
    Vec3 helper = (std::abs(w.x) > 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);

    // 3. Create the "X" (u) and "Y" (v) axes using cross products
    Vec3 v = Vec3::Cross(w, helper).Normalized();
    Vec3 u = Vec3::Cross(w, v);

    // 4. Combine them: (x * Tangent) + (y * Bitangent) + (z * Normal)
    // This is essentially a 3x3 matrix multiplication
    return u * local_dir.x + v * local_dir.y + w * local_dir.z;
}


Vec3 sample_ggx_direction(Vec3 normal, Vec3 view_dir, float roughness) {
    float alpha = roughness * roughness; // PBR convention: square roughness
    float r1 = randomGen.GetNext();
    float r2 = randomGen.GetNext();

    // 1. Calculate the 'tilt' (theta) and 'rotation' (phi) of the micro-normal
    float phi = 2.0f * PI * r1;
    float cos_theta = sqrt((1.0f - r2) / (1.0f + (alpha * alpha - 1.0f) * r2));
    float sin_theta = sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));

    // 2. Create the micro-normal (h) in local space
    Vec3 h_local(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    // 3. Transform h to world space
    Vec3 h = align_to_normal(h_local, normal);

    // 4. Reflect the view_dir across the sampled micro-normal h
    // (Standard reflect formula: 2 * dot(V, H) * H - V)
    return (h * 2.0f * Vec3::Dot(view_dir, h) - view_dir).Normalized();
}


Vec3 sample_cosine_hemisphere(const Vec3& normal) {
    // 1. Get two random numbers in [0, 1)
    float r1 = randomGen.GetNext();
    float r2 = randomGen.GetNext();

    // 2. Map to a disk (Polar coordinates)
    float r = sqrt(r1);
    float phi = 2.0f * PI * r2;

    // 3. Project disk to Hemisphere (Z is UP in local space)
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(std::max(0.0f, 1.0f - r1)); // Pythagorean theorem: x^2 + y^2 + z^2 = 1

    Vec3 local_dir(x, y, z);

    // 4. Align the local Z-up vector with the actual surface normal
    return align_to_normal(local_dir, normal);
}


Vec3 sample_pbr_direction(const Material& m, Vec3 normal, Vec3 view_dir) {
    float p = randomGen.GetNext();
    float specular_chance = lerp(0.04f, 1.0f, m.metallic); // Simplified PBR logic

    if (p < specular_chance) {
        // Sample the "Glossy" lobe (GGX)
        return sample_ggx_direction(normal, view_dir, m.roughness);
    }
    else {
        // Sample the "Diffuse" lobe (Cosine-weighted)
        return sample_cosine_hemisphere(normal);
    }
}

// beta = 2.0 is the standard choice suggested by Eric Veach
float power_heuristic(float pdf_f, float pdf_g) {
    float f2 = pdf_f * pdf_f;
    float g2 = pdf_g * pdf_g;
    return f2 / (f2 + g2);
}

LightSample sample_point_light(Vec3 hit_point, const PointLight& light) {
    Vec3 to_light = light.position - hit_point;
    float dist_sq = Vec3::Dot(to_light, to_light);
    float dist = sqrt(dist_sq);
    Vec3 L = to_light / dist;

    LightSample sample;
    // 1. The PDF is 1.0 because we always pick the only light in the scene
    sample.pdf = 1.0f;

    // 2. The radiance is the intensity divided by the distance squared
    // This is the "close enough" estimate for point lights
    sample.radiance = light.color * (light.intensity / dist_sq);

    sample.dir = L;
    sample.distance = dist;

    return sample;
}
