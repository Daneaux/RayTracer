#pragma once
#include "Material.h"
#include <math/MathTypes.h>
#include <memory>


class WorldObject {
	public:
	WorldObject() = default;
	WorldObject(
		const Mat4& worldTransform, 
		const Material& material) :	material(material)
	{
		SetWorldTransform(worldTransform);
	}

	const Mat4& GetWorldTransform() { return worldTransform; }
	void SetWorldTransform(const Mat4& value) 
	{
		worldTransform = value;
		inverseWorldTransform = worldTransform.Inverted();
	}
	const Material& GetMaterial() const { return material; }

	virtual bool IsQuad() const { return false; }
	virtual bool IsHitByRay(const Ray3& ray, Vec3& outA, float& outT, Vec3& normalA, Vec3& outB, Vec3& normalB) const = 0;

//protected:
	Mat4 worldTransform;
	Material material;
	Mat4 inverseWorldTransform;
};

class SphereObject : public WorldObject
{
public:
	SphereObject(
		float radius,
		const Mat4& worldTransform,
		const Material& material)
		: WorldObject(worldTransform, material), radius(radius)
	{
	}

	float GetRadius() const { return radius; }

	// Make IsHitByRay const
	bool IsHitByRay(const Ray3& ray, Vec3& outA, float& outT, Vec3& normalA, Vec3& outB, Vec3& normalB) const override {
		Ray3 objectRay = {
			inverseWorldTransform.Transform(Vec4(ray.origin, 1.0f)).ToVec3Drop(),
			inverseWorldTransform.Transform(Vec4(ray.direction, 0.0f)).ToVec3Drop().Normalized()
		};

		//
		// do intersection test in object space (sphere at origin with radius 'radius')
		//

		// a = dot(D, D) where D is ray direction
		float a = Vec3::Dot(objectRay.direction, objectRay.direction);

		// b = 2 * dot(D, O - C) where D is ray direction, O is ray origin, C is sphere center (0,0,0)
		float b = 2.0f * Vec3::Dot(objectRay.direction, objectRay.origin);

		// c = dot(O - C, O - C) - r^2 where O is ray origin, C is sphere center, r is sphere radius
		// by definition since sphere is at origin, O - C = O
		float c = Vec3::Dot(objectRay.origin, objectRay.origin) - radius * radius;

		// d = b^2 - 4ac (discriminant)
		float d = b * b - 4.0f * a * c;

		if (d <= 0) {
			return false;
		}
		else {
			float sqrtD = std::sqrtf(d);
			float t1 = (-b - sqrtD) / (2.0f * a);
			float t2 = (-b + sqrtD) / (2.0f * a);

			// if one or both are behind ray origin, that means ray origin is inside the sphere or sphere behind ray origin.
			if (t1 < 0.001f && t2 > 0.0001f)
			{
				// inside this object (sphere)
				outT = t2;
				Vec3 hitPointA = objectRay.origin + objectRay.direction * t2;
				outA = worldTransform.Transform(Vec4(hitPointA, 1.0f)).ToVec3Drop();
				Vec3 localNormalA = hitPointA.Normalized(); // normal points in
				normalA = worldTransform.Transform(Vec4(localNormalA, 0.0f)).ToVec3Drop().Normalized();
				return true;
			}
			if (t1 < 0.001f || t2 < 0.001f) return false;
			if (t2 < t1) std::swap(t1, t2); // ensure t1 is closer hit

			outT = t1; // return closer hit t

			// Get intersection point in object space then transform to world space
			Vec3 hitPointA = objectRay.origin + objectRay.direction * t1;
			Vec3 hitPointB = objectRay.origin + objectRay.direction * t2;
			outA = worldTransform.Transform(Vec4(hitPointA, 1.0f)).ToVec3Drop();
			outB = worldTransform.Transform(Vec4(hitPointB, 1.0f)).ToVec3Drop();

			// Get normal to hit point in object space then transofmr to world space
			// since sphere is at origin, normal is just normalized hit point
			Vec3 localNormalA = hitPointA.Normalized();
			Vec3 localNormalB = hitPointB.Normalized();
			normalA = worldTransform.Transform(Vec4(localNormalA, 0.0f)).ToVec3Drop().Normalized();
			normalB = worldTransform.Transform(Vec4(localNormalB, 0.0f)).ToVec3Drop().Normalized();

			return true;
		}
	}

private:
	float radius = 1.0f;
};

class QuadObject : public WorldObject
{
public:
	QuadObject(
		float width, 
		float height,
		const Mat4& worldTransform,
		const Material& material)
		: WorldObject(worldTransform, material), width(width), height(height)
	{
	}

	bool IsQuad() const override { return true; }

	bool useCheckeredPattern = false;
	Vec3 checkerColor1 = Vec3(1.0f, 1.0f, 1.0f);
	Vec3 checkerColor2 = Vec3(0.0f, 0.0f, 0.0f);
	float checkerScale = 2.0f;

	Vec3 GetColorAtPoint(const Vec3& worldPoint) const {
		if (!useCheckeredPattern) {
			return material.baseColor;
		}
		Vec3 localPoint = inverseWorldTransform.Transform(Vec4(worldPoint, 1.0f)).ToVec3Drop();
		int x = (int)std::floor(localPoint.x * checkerScale);
		int y = (int)std::floor(localPoint.y * checkerScale);
		return ((x + y) % 2 == 0) ? checkerColor1 : checkerColor2;
	}

	bool IsHitByRay(const Ray3& ray, Vec3& outA, float& outT, Vec3& normalA, Vec3& outB, Vec3& normalB) const override {
		Ray3 objectRay = {
			inverseWorldTransform.Transform(Vec4(ray.origin, 1.0f)).ToVec3Drop(),
			inverseWorldTransform.Transform(Vec4(ray.direction, 0.0f)).ToVec3Drop().Normalized()
		};

		//
		// do intersection test in object space (quad at origin x y aligned. Normal faces +z. Extents are width/2 and height/2)
		//

		// Ray-plane intersection: 
		// t = -dot(N, O) / dot(N, D) where N is plane normal (0,0,1), O is ray origin, D is ray direction

		Vec3 planeNormal = { 0, 0, 1 };
		float denominator = Vec3::Dot(planeNormal, objectRay.direction);
		if (std::fabs(denominator) < 0.001f) return false; // ray is parallel to plane

		float t = -Vec3::Dot(planeNormal, objectRay.origin) / denominator;
		if (t < 0.001f) return false; // plane is behind ray origin

		// Get intersection point in object space then transform to world space
		Vec3 hitPoint = objectRay.origin + objectRay.direction * t;
		if (std::fabs(hitPoint.x) > width * 0.5f || std::fabs(hitPoint.y) > height * 0.5f) return false; // hit point is outside quad bounds

		outT = t;
		outA = worldTransform.Transform(Vec4(hitPoint, 1.0f)).ToVec3Drop();
		normalA = worldTransform.Transform(Vec4(planeNormal, 0.0f)).ToVec3Drop().Normalized();

		return true;
	}

//private:
	float width, height;
};
