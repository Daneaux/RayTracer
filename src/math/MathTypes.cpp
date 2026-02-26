#include "math/MathTypes.h"
#include <cstring>

using namespace DirectX;

Mat4::Mat4() {
    memset(m, 0, sizeof(m));
}

Mat4 Mat4::Identity() {
    Mat4 r;
    r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
    return r;
}

Mat4 Mat4::LookAtLH(const Vec3& eye, const Vec3& target, const Vec3& up) {
    XMVECTOR e = XMVectorSet(eye.x, eye.y, eye.z, 1.0f);
    XMVECTOR t = XMVectorSet(target.x, target.y, target.z, 1.0f);
    XMVECTOR u = XMVectorSet(up.x, up.y, up.z, 0.0f);
    return FromXMMATRIX(XMMatrixLookAtLH(e, t, u));
}

Mat4 Mat4::PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) {
    return FromXMMATRIX(XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ));
}

Mat4 Mat4::Translation(const Vec3& t) {
    Mat4 r = Identity();
    r.m[3][0] = t.x;
    r.m[3][1] = t.y;
    r.m[3][2] = t.z;
    return r;
}

Mat4 Mat4::Scaling(float s) {
    Mat4 r;
    r.m[0][0] = s;
    r.m[1][1] = s;
    r.m[2][2] = s;
    r.m[3][3] = 1.0f;
    return r;
}

Mat4 Mat4::Scaling(const Vec3& s) {
    Mat4 r;
    r.m[0][0] = s.x;
    r.m[1][1] = s.y;
    r.m[2][2] = s.z;
    r.m[3][3] = 1.0f;
    return r;
}

Mat4 Mat4::operator*(const Mat4& b) const {
    return FromXMMATRIX(XMMatrixMultiply(ToXMMATRIX(), b.ToXMMATRIX()));
}

Mat4 Mat4::Transposed() const {
    return FromXMMATRIX(XMMatrixTranspose(ToXMMATRIX()));
}

Mat4 Mat4::Inverted() const {
    XMVECTOR det;
    return FromXMMATRIX(XMMatrixInverse(&det, ToXMMATRIX()));
}

Vec4 Mat4::Transform(const Vec4& v) const {
    XMVECTOR xv = XMVectorSet(v.x, v.y, v.z, v.w);
    XMVECTOR result = XMVector4Transform(xv, ToXMMATRIX());
    Vec4 r;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&r), result);
    return r;
}

XMMATRIX Mat4::ToXMMATRIX() const {
    return XMMATRIX(&m[0][0]);
}

Mat4 Mat4::FromXMMATRIX(const XMMATRIX& xm) {
    Mat4 r;
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&r.m), xm);
    return r;
}
