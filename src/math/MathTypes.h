#pragma once

#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator*(const Vec3& b) const { return {x * b.x, y * b.y, z * b.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSq() const { return x * x + y * y + z * z; }

    Vec3 Normalized() const {
        float len = Length();
        if (len < 1e-8f) return {0, 0, 0};
        float inv = 1.0f / len;
        return {x * inv, y * inv, z * inv};
    }

    static float Dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 Cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

struct Ray3 {
    Vec3 origin;
    Vec3 direction;
    Ray3() : origin(), direction() {}
    Ray3(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

struct Vec4 {
    float x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    inline Vec3 ToVec3Drop() const { return { x, y, z }; }

    Vec3 ToVec3Homogenous() const {
        if(w == 0) return { x, y, z }; // direction vector
        return { x / w, y / w, z / w }; // point vector (perspective divide)
    }
};

struct Mat4 {
    float m[4][4];

    Mat4();
    static Mat4 Identity();
    static Mat4 LookAtLH(const Vec3& eye, const Vec3& target, const Vec3& up);
    static Mat4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ);
    static Mat4 Translation(const Vec3& t);
    static Mat4 Scaling(float s);
    static Mat4 Scaling(const Vec3& s);

    Mat4 operator*(const Mat4& b) const;
    Mat4 Transposed() const;
    Mat4 Inverted() const;

    Vec4 Transform(const Vec4& v) const;

    DirectX::XMMATRIX ToXMMATRIX() const;
    static Mat4 FromXMMATRIX(const DirectX::XMMATRIX& xm);
};
