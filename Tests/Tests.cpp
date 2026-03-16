#include "pch.h"
#include "CppUnitTest.h"

//#include "../src/renderer/utils.h"
#include "../src/math/MathTypes.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void FresnelSchlick_v3(const Vec3& incident, const Vec3& normal, float n1, float n2, float& R, float& T)
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

void FresnelSchlick(
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

    if (cosI < 0) {
        n = -normal;
        cosI = -cosI;
        std::swap(n1, n2);
    }

    float eta = n1 / n2;
    float sinT2 = eta * eta * (1.0f - cosI * cosI);

    if (sinT2 > 1.0f) return false;

    float cosT = std::sqrt(1.0f - sinT2);

    refractedDir = (incident * eta + n * (eta * cosI - cosT)).Normalized();
    return true;
}

namespace Tests
{
	TEST_CLASS(Tests)
	{
	public:

        TEST_METHOD(TestMethod1)
        {
            float R, T;
            float PI = 3.141592653589793f;

            // Test 1: Normal Incidence (R0 check)
            FresnelSchlick(Vec3(0, -1, 0), Vec3(0, 1, 0), 1.0f, 1.5f, R, T);
            Assert::IsTrue(std::abs(R - 0.04f) < 0.001f);
            Assert::IsTrue(std::abs(T - 0.96f) < 0.001f);
            Assert::IsTrue(std::abs(1.0f - (R + T)) < 0.001f);

            // Test 2: Grazing Angle (High reflection)
            FresnelSchlick(Vec3(0.9999f, -0.0001f, 0.0f).Normalized(), Vec3(0, 1, 0), 1.0f, 1.5f, R, T);
            Assert::IsTrue(R > 0.99f);
            Assert::IsTrue(T < 0.001f);

            // Test 3: Total Internal Reflection
            float angleRad = 60.0f * PI / 180.0f;
            Vec3 I = { std::sin(angleRad), -std::cos(angleRad), 0.0 };
            Vec3 N = { 0.0, 1.0, 0.0 };
            float n1 = 1.5f; // Glass
            float n2 = 1.0f; // Air
            FresnelSchlick(I, N, n1, n2, R, T);
            Assert::IsTrue(R > 0.999f);
            Assert::IsTrue(T < 0.001f);

            // test 4: same medium
            I = { 0.0, -1.0, 0.0 };
            N = { 0.0, 1.0, 0.0 };
            n1 = 1.33; // Water
            n2 = 1.33; // Water
            R = -1.0, T = -1.0; // Init with garbage values
            FresnelSchlick(I, N, n1, n2, R, T);
            Assert::IsTrue(R < 0.0001f);
            Assert::IsTrue(T > 0.9999f);

            // test 5: glass to air
            // Glass to air at 40 degrees (just below critical angle of 41.8 deg)
            // At steep angles near TIR, reflectance should be high but < 1.0
            I = Vec3(std::sin(40.0f * PI / 180.0f), -std::cos(40.0f * PI / 180.0f), 0.0f).Normalized();
            N = { 0.0, 1.0, 0.0 };
            n1 = 1.5f; // glass
            n2 = 1.0f; // air
            FresnelSchlick(I, N, n1, n2, R, T);
            Assert::IsTrue(R > 0.2f);      // well above R0=0.04 (exact Fresnel ≈ 0.245)
            Assert::IsTrue(R < 1.0f);      // not yet TIR
            Assert::IsTrue(std::abs(R + T - 1.0f) < 0.001f); // energy conservation
        }

        TEST_METHOD(RefractRay_NormalIncidence_NoBending)
        {
            // Ray straight down onto surface: should pass through without bending
            Vec3 incident(0, -1, 0);
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            bool result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsTrue(result);

            // Refracted ray should still point straight down
            Assert::IsTrue(std::abs(refracted.x) < 0.001f);
            Assert::IsTrue(refracted.y < -0.99f);
            Assert::IsTrue(std::abs(refracted.z) < 0.001f);
        }

        TEST_METHOD(RefractRay_AirToGlass_SnellsLaw)
        {
            // Air (n=1.0) to Glass (n=1.5) at 30 degrees from normal
            // Snell: sin(theta_t) = (1.0/1.5) * sin(30) = 0.333
            // theta_t = 19.47 degrees
            float PI = 3.141592653589793f;
            float angle = 30.0f * PI / 180.0f;
            Vec3 incident(std::sin(angle), -std::cos(angle), 0.0f);
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            bool result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsTrue(result);

            // Check refracted angle via sin(theta_t) = |refracted.x|
            float sinRefracted = std::abs(refracted.x);
            float expectedSin = (1.0f / 1.5f) * std::sin(angle); // ~0.333
            Assert::IsTrue(std::abs(sinRefracted - expectedSin) < 0.01f);

            // Refracted ray should bend toward normal (smaller angle), still going down
            Assert::IsTrue(refracted.y < 0.0f);
            // x should be positive (same side as incident)
            Assert::IsTrue(refracted.x > 0.0f);
        }

        TEST_METHOD(RefractRay_GlassToAir_BelowCriticalAngle)
        {
            // Glass (n=1.5) to Air (n=1.0) at 10 degrees
            // Critical angle for glass->air = arcsin(1.0/1.5) = 41.8 deg
            // 10 deg is well below, should refract
            // Snell: sin(theta_t) = (1.5/1.0) * sin(10) = 0.260
            // theta_t = 15.1 degrees
            float PI = 3.141592653589793f;
            float angle = 10.0f * PI / 180.0f;

            // Ray going upward from inside (same dir as normal), hitting surface from inside
            Vec3 incident(std::sin(angle), std::cos(angle), 0.0f); // going up and slightly right
            Vec3 normal(0, 1, 0); // outward-facing normal

            Vec3 refracted;
            bool result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsTrue(result);

            // Refracted ray bends away from normal (larger angle), going upward
            Assert::IsTrue(refracted.y > 0.0f);
            float sinRefracted = std::abs(refracted.x);
            float expectedSin = (1.5f / 1.0f) * std::sin(angle); // ~0.260
            Assert::IsTrue(std::abs(sinRefracted - expectedSin) < 0.01f);
        }

        TEST_METHOD(RefractRay_TotalInternalReflection)
        {
            // Glass (n=1.5) to Air (n=1.0)
            // Critical angle = arcsin(1.0/1.5) = 41.8 degrees
            // At 60 degrees (above critical angle) => TIR, should return false
            float PI = 3.141592653589793f;
            float angle = 60.0f * PI / 180.0f;

            // Ray going upward from inside at steep angle
            Vec3 incident(std::sin(angle), std::cos(angle), 0.0f);
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            bool result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsFalse(result);
        }

        TEST_METHOD(RefractRay_SameMedium_NoBending)
        {
            // Same IOR on both sides: ray should pass through unchanged
            float PI = 3.141592653589793f;
            float angle = 45.0f * PI / 180.0f;
            Vec3 incident(std::sin(angle), -std::cos(angle), 0.0f);
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            bool result = RefractRay(incident, normal, 1.5f, 1.5f, refracted);
            Assert::IsTrue(result);

            // Direction should be unchanged
            Vec3 incNorm = incident.Normalized();
            Assert::IsTrue(std::abs(refracted.x - incNorm.x) < 0.001f);
            Assert::IsTrue(std::abs(refracted.y - incNorm.y) < 0.001f);
            Assert::IsTrue(std::abs(refracted.z - incNorm.z) < 0.001f);
        }

        TEST_METHOD(RefractRay_OutputIsNormalized)
        {
            // Verify the output vector is unit length
            float PI = 3.141592653589793f;
            float angle = 45.0f * PI / 180.0f;
            Vec3 incident(std::sin(angle), -std::cos(angle), 0.0f);
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            float len = refracted.Length();
            Assert::IsTrue(std::abs(len - 1.0f) < 0.001f);
        }

        TEST_METHOD(RefractRay_Reversibility)
        {
            // Refracting air->glass then glass->air should recover
            // approximately the original direction
            float PI = 3.141592653589793f;
            float angle = 25.0f * PI / 180.0f;
            Vec3 incident(std::sin(angle), -std::cos(angle), 0.0f);
            Vec3 normal(0, 1, 0);

            // Air to glass (entering, ray goes down)
            Vec3 refracted1;
            bool r1 = RefractRay(incident, normal, 1.0f, 1.5f, refracted1);
            Assert::IsTrue(r1);

            // Glass to air (exiting, ray still going down, hitting bottom surface)
            // At bottom surface, normal points down (outward from sphere bottom)
            Vec3 bottomNormal(0, -1, 0);
            Vec3 refracted2;
            bool r2 = RefractRay(refracted1, bottomNormal, 1.0f, 1.5f, refracted2);
            Assert::IsTrue(r2);

            // Should recover original direction
            Vec3 incNorm = incident.Normalized();
            Assert::IsTrue(std::abs(refracted2.x - incNorm.x) < 0.01f);
            Assert::IsTrue(std::abs(refracted2.y - incNorm.y) < 0.01f);
        }

        TEST_METHOD(RefractRay_AtCriticalAngle)
        {
            // Exactly at critical angle: sin(theta_c) = n2/n1 = 1.0/1.5
            // theta_c = arcsin(0.6667) = 41.81 degrees
            // At this angle refraction should just barely work, with refracted
            // ray going nearly parallel to surface (theta_t ~ 90 deg)
            float n1_inside = 1.5f;
            float n2_outside = 1.0f;
            float criticalAngle = std::asin(n2_outside / n1_inside); // ~0.7297 rad

            // Slightly below critical angle (should refract)
            float belowCritical = criticalAngle - 0.01f;
            Vec3 incident(std::sin(belowCritical), std::cos(belowCritical), 0.0f);
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            bool result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsTrue(result);

            // Slightly above critical angle (should TIR)
            float aboveCritical = criticalAngle + 0.01f;
            incident = Vec3(std::sin(aboveCritical), std::cos(aboveCritical), 0.0f);
            result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsFalse(result);
        }

        TEST_METHOD(RefractRay_3DAngle)
        {
            // Verify refraction works in 3D, not just the XY plane
            Vec3 incident(0.3f, -0.8f, 0.2f);
            incident = incident.Normalized();
            Vec3 normal(0, 1, 0);
            Vec3 refracted;

            bool result = RefractRay(incident, normal, 1.0f, 1.5f, refracted);
            Assert::IsTrue(result);

            // Verify Snell's law: n1*sin(theta_i) = n2*sin(theta_t)
            float cosI = std::abs(Vec3::Dot(incident, normal));
            float sinI = std::sqrt(1.0f - cosI * cosI);
            float cosT = std::abs(Vec3::Dot(refracted, normal));
            float sinT = std::sqrt(1.0f - cosT * cosT);

            float lhs = 1.0f * sinI;
            float rhs = 1.5f * sinT;
            Assert::IsTrue(std::abs(lhs - rhs) < 0.01f);

            // Refracted ray should be in the same half-plane as incident
            Assert::IsTrue(refracted.y < 0.0f);
        }
	};
}
