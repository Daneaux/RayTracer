#pragma once

#include "Scene.h"
#include "Camera.h"
#include <cmath>
#include <algorithm>
#include "Object.h"
#include <cassert>
#include <random>

inline int MAX_BOUNCES = 4;

class RandomGen
{
public:
    std::random_device              rd;
    std::mt19937                    randomGenerator;
    std::uniform_real_distribution<> randomDistribution;

    RandomGen()
    {
        randomGenerator = std::mt19937(rd());
        randomDistribution = std::uniform_real_distribution<>(0.0, 1.0);
    }

    float GetNext()
    {
        return randomDistribution(randomGenerator);
    }
};

inline RandomGen randomGen;

struct LightSample {
    Vec3 radiance;
    Vec3 dir;
    float distance;
    float pdf;
};

struct Intersection {
    bool occurred = false;  // Did we hit anything?
    float distance = 1e30f; // Distance from ray origin to hit point
    Vec3 point;             // The exact 3D coordinates of the hit
    Vec3 normal;            // The surface normal at the hit point
    Material material;      // The material properties of the object hit
};

struct LightHitDirTuple
{
    Light& light;
    Vec3 surfaceToLightNormalized;
    float lightDistance;
};

struct ShadingComponents {
    Vec3 diffuse;
    Vec3 specular;
};

template <typename T>
constexpr T lerp(T a, T b, T t);

static Vec3 GetAverageColor(const std::vector<Vec3>& colors);

// Function to calculate reflection
Vec3 ReflectRay(const Vec3& incident, const Vec3& normal);

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
    Vec3& refractedDir);

// At this angle, outReflectance means:
// 0.0 = fully refracted (perfectly transparent), 1.0 = fully reflected (perfect mirror).
// In practice, the actual reflected color would be the incoming ray color multiplied by outReflectance, 
// and the refracted color would be the incoming ray color multiplied by (1.0f - outReflectance).
void  FresnelSchlick_v2(
    Vec3& incomingRayNormalized,
    Vec3& normalNormalized,
    float n1, float n2,
    float& outReflectance,
    float& outTransmittance);

void FresnelSchlick(const Vec3& incident, const Vec3& normal, float n1, float n2, float& R, float& T);

Vec3 align_to_normal(const Vec3& local_dir, const Vec3& normal);
Vec3 sample_ggx_direction(Vec3 normal, Vec3 view_dir, float roughness);
Vec3 sample_cosine_hemisphere(const Vec3& normal);
Vec3 sample_pbr_direction(const Material& m, Vec3 normal, Vec3 view_dir);

float power_heuristic(float pdf_f, float pdf_g);

LightSample sample_point_light(Vec3 hit_point, const PointLight& light);

Vec3 LambertShadingModel(LightHitDirTuple& tuple, Vec3& normalA, WorldObject* obj);
Vec3 BlinnPhong(LightHitDirTuple& tuple, Vec3& hitNormal, Vec3& viewDir, Material& mat);
Vec3 BlinnPhongWithLightAttenuation(LightHitDirTuple& tuple, Vec3& hitNormal, Vec3& viewDir, Material& mat);
ShadingComponents BlinnPhongSeparated(LightHitDirTuple& tuple, Vec3& hitNormal, Vec3& viewDir, Material& mat);

