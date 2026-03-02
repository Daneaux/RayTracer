#include "renderer/SoftwareRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <cmath>
#include <algorithm>

bool SoftwareRenderer::Initialize(DXDevice& device, uint32_t width, uint32_t height) {
    m_bufWidth = width;
    m_bufHeight = height;
    m_pixelBuffer.resize(width * height, 0xFF000000);

    m_quad = std::make_unique<FullscreenQuad>();
    return m_quad->Initialize(device, width, height);
}

void SoftwareRenderer::OnResize(DXDevice& device, uint32_t width, uint32_t height) {
    // Buffer will be resized in Render if target dimensions differ
    (void)device;
    (void)width;
    (void)height;
}

void SoftwareRenderer::Render(DXDevice& device, const Scene& scene,
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
            Vec3 color = TraceRay(origin, direction, scene, screenU, screenV);

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

Vec3 SoftwareRenderer::TraceRay(const Vec3& origin, const Vec3& direction,
                                 const Scene& scene,
                                 float screenU, float screenV) const {
    float t;
    if (IntersectSphere(origin, direction, scene.sphere, t)) {
        Vec3 hitPoint = origin + direction * t;
        Vec3 normal = (hitPoint - scene.sphere.center) * (1.0f / scene.sphere.radius);
        Vec3 viewDir = (origin - hitPoint).Normalized();
        return ComputePhongLighting(hitPoint, normal, viewDir,
                                     scene.sphere, scene.light, scene.ambientColor);
    }

    // Per-pixel gradient background: R = x/width, G = y/height, B = 0.5
    return Vec3(screenU, screenV, 0.5f);
}

bool SoftwareRenderer::IntersectSphere(const Vec3& origin, const Vec3& dir,
                                        const Sphere& sphere, float& t) const {
    Vec3 L = origin - sphere.center;
    float a = Vec3::Dot(dir, dir);
    float b = 2.0f * Vec3::Dot(dir, L);
    float c = Vec3::Dot(L, L) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0) return false;

    float sqrtD = std::sqrt(discriminant);
    float t0 = (-b - sqrtD) / (2.0f * a);
    float t1 = (-b + sqrtD) / (2.0f * a);

    if (t0 > 0.001f) { t = t0; return true; }
    if (t1 > 0.001f) { t = t1; return true; }
    return false;
}

Vec3 SoftwareRenderer::ComputePhongLighting(const Vec3& hitPoint, const Vec3& normal,
                                             const Vec3& viewDir, const Sphere& sphere,
                                             const PointLight& light,
                                             const Vec3& ambient) const {
    Vec3 L = (light.position - hitPoint).Normalized();
    Vec3 H = (L + viewDir).Normalized();

    float diff = std::max(Vec3::Dot(normal, L), 0.0f);
    float spec = std::pow(std::max(Vec3::Dot(normal, H), 0.0f), sphere.specularPower);

    Vec3 ambientTerm = ambient * sphere.color;
    Vec3 diffuseTerm = sphere.color * light.color * (diff * light.intensity);
    Vec3 specularTerm = light.color * (spec * light.intensity);

    return {
        ambientTerm.x + diffuseTerm.x + specularTerm.x,
        ambientTerm.y + diffuseTerm.y + specularTerm.y,
        ambientTerm.z + diffuseTerm.z + specularTerm.z
    };
}
