#pragma once
#include <Application.h>
#include "math/MathTypes.h" // Add this include to resolve Vec3

class Light 
{
public:
	Light() = default;
	Light(Vec3 position, Vec3 color, float intensity) : position(position), color(color), intensity(intensity) {}
	virtual ~Light() = default;

	Vec3 color;
	Vec3 position;
	float intensity;
};

class PointLight : public Light {
public:
	PointLight(const Vec3& position, const Vec3& color, float intensity)
		: Light(position, color, intensity)
	{
		this->color = color;
		this->position = position;
	}
};

class DirectionalLight : public Light {
public:
	DirectionalLight(Vec3 direction, Vec3 color, float intensity)
		: Light(Vec3(0, 0, 0), color, intensity),  direction(direction.Normalized())
	{
	}
	Vec3 direction;
};

class SquareAreaLight : public Light {
	public:
	SquareAreaLight(Vec3 position, Vec3 color, Vec3 direction, float intensity, float size)
		: Light(position, color, intensity), direction(direction), size(size)
	{
	}
	float size;
	Vec3 direction;
};
