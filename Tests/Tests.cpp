#include "pch.h"
#include "CppUnitTest.h"

//#include "../src/renderer/utils.h"
#include "../src/math/MathTypes.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
        }
	};
}
