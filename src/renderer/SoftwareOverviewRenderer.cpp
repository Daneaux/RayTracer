#include "renderer/SoftwareOverviewRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "SwapChainTarget.h"
#include <cmath>
#include <algorithm>

static const float NEAR_CLIP_W = 0.001f;

// ---------------------------------------------------------------
// Pixel helpers
// ---------------------------------------------------------------

uint32_t SoftwareOverviewRenderer::ColorToABGR(const Vec4& color) const {
    uint8_t r = (uint8_t)(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
    uint8_t g = (uint8_t)(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
    uint8_t b = (uint8_t)(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
    uint8_t a = (uint8_t)(std::clamp(color.w, 0.0f, 1.0f) * 255.0f);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void SoftwareOverviewRenderer::SetPixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < (int)m_bufWidth && y >= 0 && y < (int)m_bufHeight) {
        m_pixelBuffer[y * m_bufWidth + x] = color;
    }
}

// ---------------------------------------------------------------
// Bresenham line rasterizer
// ---------------------------------------------------------------

void SoftwareOverviewRenderer::DrawLine2D(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        SetPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// ---------------------------------------------------------------
// 3D line: project through VP, clip against near plane, rasterize
// ---------------------------------------------------------------

void SoftwareOverviewRenderer::DrawLine3D(const Vec3& a, const Vec3& b,
                                           const Vec4& color, const Mat4& vp) {
    Vec4 clipA = vp.Transform(Vec4(a.x, a.y, a.z, 1.0f));
    Vec4 clipB = vp.Transform(Vec4(b.x, b.y, b.z, 1.0f));

    bool aVis = clipA.w > NEAR_CLIP_W;
    bool bVis = clipB.w > NEAR_CLIP_W;

    if (!aVis && !bVis) return; // both behind camera

    // Clip the behind-camera endpoint to w = NEAR_CLIP_W
    if (!aVis) {
        float t = (NEAR_CLIP_W - clipA.w) / (clipB.w - clipA.w);
        clipA.x = clipA.x + t * (clipB.x - clipA.x);
        clipA.y = clipA.y + t * (clipB.y - clipA.y);
        clipA.z = clipA.z + t * (clipB.z - clipA.z);
        clipA.w = NEAR_CLIP_W;
    } else if (!bVis) {
        float t = (NEAR_CLIP_W - clipB.w) / (clipA.w - clipB.w);
        clipB.x = clipB.x + t * (clipA.x - clipB.x);
        clipB.y = clipB.y + t * (clipA.y - clipB.y);
        clipB.z = clipB.z + t * (clipA.z - clipB.z);
        clipB.w = NEAR_CLIP_W;
    }

    // Perspective divide → NDC
    float ndcAx = clipA.x / clipA.w;
    float ndcAy = clipA.y / clipA.w;
    float ndcBx = clipB.x / clipB.w;
    float ndcBy = clipB.y / clipB.w;

    // Viewport transform (flip Y for screen coords)
    float fw = (float)m_bufWidth;
    float fh = (float)m_bufHeight;
    int sx0 = (int)((ndcAx + 1.0f) * 0.5f * fw);
    int sy0 = (int)((1.0f - ndcAy) * 0.5f * fh);
    int sx1 = (int)((ndcBx + 1.0f) * 0.5f * fw);
    int sy1 = (int)((1.0f - ndcBy) * 0.5f * fh);

    DrawLine2D(sx0, sy0, sx1, sy1, ColorToABGR(color));
}

// ---------------------------------------------------------------
// Scene element drawing
// ---------------------------------------------------------------

void SoftwareOverviewRenderer::DrawGrid(const Mat4& vp) {
    const float gridSize = 5.0f;
    const float step = 1.0f;

    Vec4 xzColor = {0.3f, 0.3f, 0.3f, 1.0f};
    Vec4 xyColor = {0.15f, 0.15f, 0.35f, 1.0f};
    Vec4 yzColor = {0.35f, 0.15f, 0.15f, 1.0f};

    // XZ plane (Y=0) - gray
    for (float i = -gridSize; i <= gridSize; i += step) {
        if (std::abs(i) < 0.01f) continue;
        DrawLine3D({i, 0, -gridSize}, {i, 0, gridSize}, xzColor, vp);
        DrawLine3D({-gridSize, 0, i}, {gridSize, 0, i}, xzColor, vp);
    }

    // XY plane (Z=0) - faint blue
    for (float i = -gridSize; i <= gridSize; i += step) {
        if (std::abs(i) < 0.01f) continue;
        DrawLine3D({i, -gridSize, 0}, {i, gridSize, 0}, xyColor, vp);
        DrawLine3D({-gridSize, i, 0}, {gridSize, i, 0}, xyColor, vp);
    }

    // YZ plane (X=0) - faint red
    for (float i = -gridSize; i <= gridSize; i += step) {
        if (std::abs(i) < 0.01f) continue;
        DrawLine3D({0, i, -gridSize}, {0, i, gridSize}, yzColor, vp);
        DrawLine3D({0, -gridSize, i}, {0, gridSize, i}, yzColor, vp);
    }

    // Axis lines
    float axisLen = gridSize;
    DrawLine3D({-axisLen, 0, 0}, {axisLen, 0, 0}, {1.0f, 0.2f, 0.2f, 1.0f}, vp); // X red
    DrawLine3D({0, -axisLen, 0}, {0, axisLen, 0}, {0.2f, 1.0f, 0.2f, 1.0f}, vp); // Y green
    DrawLine3D({0, 0, -axisLen}, {0, 0, axisLen}, {0.2f, 0.2f, 1.0f, 1.0f}, vp); // Z blue
}

void SoftwareOverviewRenderer::DrawSphereWireframe(
    const SphereObject& sphere, 
    const Mat4& vp) 
{
    const uint32_t slices = 16;
    const uint32_t stacks = 8;
	Material mat = sphere.GetMaterial();
    Vec4 color = {mat.baseColor, 1.0f};

    auto vtx = [&](uint32_t i, uint32_t j) -> Vec3 {
        float phi   = PI * (float)i / (float)stacks;
        float theta = 2.0f * PI * (float)j / (float)slices;
        float sp = std::sin(phi), cp = std::cos(phi);
        Vec3 local = {sp * std::cos(theta), cp, sp * std::sin(theta)};
       // return sphere.center + local * sphere.radius;
        return local * sphere.GetRadius();
    };

    // Horizontal rings
    for (uint32_t i = 0; i <= stacks; ++i)
        for (uint32_t j = 0; j < slices; ++j)
            DrawLine3D(vtx(i, j), vtx(i, j + 1), color, vp);

    // Vertical arcs
    for (uint32_t j = 0; j <= slices; ++j)
        for (uint32_t i = 0; i < stacks; ++i)
            DrawLine3D(vtx(i, j), vtx(i + 1, j), color, vp);
}

void SoftwareOverviewRenderer::DrawLightIndicator(PointLight& light, const Mat4& vp) {
    Vec4 color = {1.0f, 1.0f, 0.0f, 1.0f};
    float s = 0.15f;
    Vec3 p = light.position;

    Vec3 px = p + Vec3{s, 0, 0};
    Vec3 nx = p + Vec3{-s, 0, 0};
    Vec3 py = p + Vec3{0, s, 0};
    Vec3 ny = p + Vec3{0, -s, 0};
    Vec3 pz = p + Vec3{0, 0, s};
    Vec3 nz = p + Vec3{0, 0, -s};

    // Top edges
    DrawLine3D(py, px, color, vp);  DrawLine3D(py, pz, color, vp);
    DrawLine3D(py, nx, color, vp);  DrawLine3D(py, nz, color, vp);
    // Bottom edges
    DrawLine3D(ny, px, color, vp);  DrawLine3D(ny, pz, color, vp);
    DrawLine3D(ny, nx, color, vp);  DrawLine3D(ny, nz, color, vp);
    // Equator
    DrawLine3D(px, pz, color, vp);  DrawLine3D(pz, nx, color, vp);
    DrawLine3D(nx, nz, color, vp);  DrawLine3D(nz, px, color, vp);
}

void SoftwareOverviewRenderer::DrawFrustum(const Camera& cam, const Mat4& vp) {
    Vec3 eye = cam.GetPosition();
    Vec3 fwd = cam.GetForward();
    float fovY   = cam.GetFovY();
    float aspect = cam.GetAspectRatio();
    float nearZ  = cam.GetNearZ();
    float farZ   = std::min(cam.GetFarZ(), 5.0f);

    Vec3 right = Vec3::Cross(fwd, Vec3(0, 1, 0)).Normalized();
    Vec3 camUp = Vec3::Cross(right, fwd).Normalized();

    float nH = std::tan(fovY * 0.5f) * nearZ,  nW = nH * aspect;
    float fH = std::tan(fovY * 0.5f) * farZ,   fW = fH * aspect;

    Vec3 nc = eye + fwd * nearZ;
    Vec3 fc = eye + fwd * farZ;

    Vec3 ntl = nc + camUp * nH - right * nW;
    Vec3 ntr = nc + camUp * nH + right * nW;
    Vec3 nbr = nc - camUp * nH + right * nW;
    Vec3 nbl = nc - camUp * nH - right * nW;

    Vec3 ftl = fc + camUp * fH - right * fW;
    Vec3 ftr = fc + camUp * fH + right * fW;
    Vec3 fbr = fc - camUp * fH + right * fW;
    Vec3 fbl = fc - camUp * fH - right * fW;

    Vec4 eyeC  = {1.0f, 1.0f, 0.0f, 1.0f};
    Vec4 nearC = {0.0f, 1.0f, 1.0f, 1.0f};
    Vec4 farC  = {0.0f, 0.7f, 0.7f, 1.0f};

    // Eye to near
    DrawLine3D(eye, ntl, eyeC, vp);  DrawLine3D(eye, ntr, eyeC, vp);
    DrawLine3D(eye, nbr, eyeC, vp);  DrawLine3D(eye, nbl, eyeC, vp);
    // Near rectangle
    DrawLine3D(ntl, ntr, nearC, vp); DrawLine3D(ntr, nbr, nearC, vp);
    DrawLine3D(nbr, nbl, nearC, vp); DrawLine3D(nbl, ntl, nearC, vp);
    // Far rectangle
    DrawLine3D(ftl, ftr, farC, vp);  DrawLine3D(ftr, fbr, farC, vp);
    DrawLine3D(fbr, fbl, farC, vp);  DrawLine3D(fbl, ftl, farC, vp);
    // Near to far
    DrawLine3D(ntl, ftl, farC, vp);  DrawLine3D(ntr, ftr, farC, vp);
    DrawLine3D(nbr, fbr, farC, vp);  DrawLine3D(nbl, fbl, farC, vp);
}

// ---------------------------------------------------------------
// Main render
// ---------------------------------------------------------------

void SoftwareOverviewRenderer::Render(
    DXDevice& device,
    Scene& scene,
    const Camera& camera,
    SwapChainTarget& target)
{
    uint32_t w = target.GetWidth();
    uint32_t h = target.GetHeight();
    if (w == 0 || h == 0) return;

    EnsureBufferSize(device, w, h);

    // Clear to dark background
    uint32_t clearCol = ColorToABGR({0.05f, 0.05f, 0.08f, 1.0f});
    std::fill(m_pixelBuffer.begin(), m_pixelBuffer.end(), clearCol);


    Light* l = scene.GetLights()[0];
    PointLight& light = dynamic_cast<PointLight&>(*l);

    Mat4 vp = camera.GetViewProjection();

    DrawGrid(vp);
   // DrawSphereWireframe(sphere, vp);
    DrawLightIndicator(light, vp);
    if (m_observedCamera) {
        DrawFrustum(*m_observedCamera, vp);
    }

    UploadAndDraw(device, target);
}
