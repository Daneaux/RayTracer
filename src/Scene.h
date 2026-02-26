#pragma once

#include "math/MathTypes.h"

struct Sphere {
    Vec3  center = {0, 0, 0};
    float radius = 1.0f;
    Vec3  color = {0.8f, 0.2f, 0.2f};
    float specularPower = 64.0f;
};

struct PointLight {
    Vec3  position = {2.0f, 3.0f, -2.0f};
    Vec3  color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};

struct Scene {
    Sphere     sphere;
    PointLight light;
    Vec3       ambientColor = {0.1f, 0.1f, 0.1f};

    void SetupDefault() {
        sphere.center = {0, 0, 0};
        sphere.radius = 1.0f;
        sphere.color = {0.8f, 0.2f, 0.2f};
        sphere.specularPower = 64.0f;

        light.position = {2.0f, 3.0f, -2.0f};
        light.color = {1.0f, 1.0f, 1.0f};
        light.intensity = 1.0f;

        ambientColor = {0.1f, 0.1f, 0.1f};
    }
};
