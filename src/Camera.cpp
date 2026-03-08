#include "Camera.h"
#include "InputManager.h"
#include <cmath>
#include <algorithm>

Camera::Camera() {
    m_view = Mat4::Identity();
    m_projection = Mat4::Identity();
}

void Camera::SetPerspective(float fovY, float aspect, float nearZ, float farZ) {
    m_fovY = fovY;
    m_aspectRatio = aspect;
    m_nearZ = nearZ;
    m_farZ = farZ;
    m_projection = Mat4::PerspectiveFovLH(fovY, aspect, nearZ, farZ);
}

void Camera::LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    m_position = eye;
    m_forward = (target - eye).Normalized();
    m_up = up;
    m_view = Mat4::LookAtLH(eye, target, up);
}

void Camera::UpdateMatrices() {
    Vec3 target = m_position + m_forward;
    m_view = Mat4::LookAtLH(m_position, target, m_up);
    m_projection = Mat4::PerspectiveFovLH(m_fovY, m_aspectRatio, m_nearZ, m_farZ);
}

void Camera::GenerateRay(float ndcX, float ndcY, Vec3& outOrigin, Vec3& outDir) const {
    Vec3 right = Vec3::Cross(m_forward, m_up).Normalized();
    Vec3 camUp = Vec3::Cross(right, m_forward).Normalized();

    float halfH = std::tan(m_fovY * 0.5f);
    float halfW = halfH * m_aspectRatio;

    outOrigin = m_position;
    outDir = (m_forward + ndcX * halfW * right + ndcY * halfH * camUp).Normalized();
}

FlyCamera::FlyCamera() {
    m_position = {0, 0, -10};
    m_forward = {0, 0, 1};
    m_yaw = 0.0f;
    m_pitch = 0.0f;
}

void FlyCamera::Update(const InputManager* input, float dt) {
    if (!input) return;

    if (input->GetPressedButton() == InputManager::Left) {
        float dx = (float)input->GetMouseDeltaX() * m_lookSensitivity;
        float dy = (float)input->GetMouseDeltaY() * m_lookSensitivity;
        m_yaw += dx;
        m_pitch -= dy;
        m_pitch = std::clamp(m_pitch, -1.5533f, 1.5533f);

        m_forward.x = std::cos(m_pitch) * std::sin(m_yaw);
        m_forward.y = std::sin(m_pitch);
        m_forward.z = std::cos(m_pitch) * std::cos(m_yaw);
        m_forward = m_forward.Normalized();
    }
    else if (input->GetPressedButton() == InputManager::Right) {
        float dx = (float)input->GetMouseDeltaX() * m_orbitSensitivity;
        float dy = (float)input->GetMouseDeltaY() * m_orbitSensitivity;

        // Get current offset from orbit target
        Vec3 offset = m_position - m_orbitTarget;
        float radius = offset.Length();
        if (radius < 0.001f) radius = 0.001f;

        // Compute current spherical angles from offset
        float orbitYaw = std::atan2(offset.x, offset.z);
        float orbitPitch = std::asin(std::clamp(offset.y / radius, -1.0f, 1.0f));

        // Apply mouse deltas
        orbitYaw += dx;
        orbitPitch += dy;
        orbitPitch = std::clamp(orbitPitch, -1.5533f, 1.5533f);

        // Recompute position on the sphere
        m_position.x = m_orbitTarget.x + radius * std::cos(orbitPitch) * std::sin(orbitYaw);
        m_position.y = m_orbitTarget.y + radius * std::sin(orbitPitch);
        m_position.z = m_orbitTarget.z + radius * std::cos(orbitPitch) * std::cos(orbitYaw);

        // Look at the target
        m_forward = (m_orbitTarget - m_position).Normalized();

        // Sync yaw/pitch so left-click free-look stays consistent
        m_yaw = std::atan2(m_forward.x, m_forward.z);
        m_pitch = std::asin(std::clamp(m_forward.y, -1.0f, 1.0f));
    }
    UpdateMatrices();
}

void FlyCamera::ApplyKeyboard(const InputManager* input, float dt) {
    if (!input) return;

    Vec3 right = Vec3::Cross(m_forward, Vec3(0, 1, 0)).Normalized();
    Vec3 up = {0, 1, 0};

    float speed = m_moveSpeed * dt;
    bool moved = false;
    if (input->IsKeyDown('W')) { m_position += m_forward * speed; moved = true; }
    if (input->IsKeyDown('S')) { m_position -= m_forward * speed; moved = true; }
    if (input->IsKeyDown('A')) { m_position -= right * speed; moved = true; }
    if (input->IsKeyDown('D')) { m_position += right * speed; moved = true; }
    if (input->IsKeyDown(VK_SPACE)) { m_position += up * speed; moved = true; }
    if (input->IsKeyDown(VK_SHIFT)) { m_position -= up * speed; moved = true; }

    if (moved) UpdateMatrices();
}

void FlyCamera::ApplyMouseWheel(const InputManager* input) {
    if (!input) return;
    int wheelDelta = input->GetMouseWheelDelta();
    if (wheelDelta != 0) {
        float scrollSpeed = 2.0f;
        m_position += m_forward * (wheelDelta / 120.0f) * scrollSpeed;
        UpdateMatrices();
    }
}
