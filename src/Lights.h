#pragma once
#include <Application.h>
#include "math/MathTypes.h" // Add this include to resolve Vec3

class Light 
{
public:
	Light() = default;

	virtual ~Light() = default;
};

class PointLight : public Light {
public:
	PointLight(Vec3 position, Vec3 color, float intensity)
		: position(position), color(color), intensity(intensity)
	{
	}
	Vec3 position;
	Vec3 color;
	float intensity;
};

class DirectionalLight : public Light {
public:
	DirectionalLight(Vec3 direction, Vec3 color, float intensity)
		: direction(direction), color(color), intensity(intensity)
	{
	}
	Vec3 direction;
	Vec3 color;
	float intensity;
};
