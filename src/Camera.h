#pragma once

#include "math/MathTypes.h"

class InputManager;

class Camera {
public:
    Camera();
    virtual ~Camera() = default;

    void SetPerspective(float fovY, float aspect, float nearZ, float farZ);
    void LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    void UpdateMatrices();

    const Mat4& GetView() const { return m_view; }
    const Mat4& GetProjection() const { return m_projection; }
    Mat4 GetViewProjection() const { return m_view * m_projection; }
    const Vec3& GetPosition() const { return m_position; }
    const Vec3& GetForward() const { return m_forward; }
    const Vec3& GetUp() const { return m_up; }
    float GetFovY() const { return m_fovY; }
    float GetAspectRatio() const { return m_aspectRatio; }
    float GetNearZ() const { return m_nearZ; }
    float GetFarZ() const { return m_farZ; }

    void GenerateRay(float ndcX, float ndcY, Vec3& outOrigin, Vec3& outDir) const;

protected:
    Vec3  m_position = {0, 0, -5};
    Vec3  m_forward  = {0, 0, 1};
    Vec3  m_up       = {0, 1, 0};
    float m_fovY = 0.7854f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearZ = 0.1f;
    float m_farZ = 100.0f;
    Mat4  m_view;
    Mat4  m_projection;
};

class FlyCamera : public Camera {
public:
    FlyCamera();
    void Update(const InputManager* input, float deltaTime);
    void ApplyKeyboard(const InputManager* input, float deltaTime);
    void ApplyMouseWheel(const InputManager* input);

    void SetOrbitTarget(const Vec3& target) { m_orbitTarget = target; }
    const Vec3& GetOrbitTarget() const { return m_orbitTarget; }

private:
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_moveSpeed = 5.0f;
    float m_lookSensitivity = 0.003f;
    float m_orbitSensitivity = 0.005f;
    Vec3  m_orbitTarget = {0, 0, 0};
};
