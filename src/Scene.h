#pragma once

#include "Lights.h"
#include <vector>
#include "math/MathTypes.h"
#include "Camera.h"
#include "Object.h"

class Light;

class Scene {
public:
    Scene()
    { }

    void AddObject(WorldObject* obj) {
        objects.push_back(obj);
    }

    void AddLight(Light* light) {
        lights.push_back(light);
    }

    const std::vector<WorldObject*>& GetObjects() const { return objects; }
    const std::vector<Light*>& GetLights() const { return lights; }
    const Vec3& GetAmbientColor() const { return ambientColor; }
    void SetAmbientColor(Vec3 c) { ambientColor = c; }

private:
    std::vector<WorldObject*> objects;
    std::vector<Light*>       lights;
    Vec3       ambientColor = {0.05f, 0.05f, 0.05f};
};
