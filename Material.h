#pragma once
#include <math/MathTypes.h>


// Future: https://math.hws.edu/graphicsbook/c8/s2.html

struct Material {

	Material() = default;
	Material(
		const Vec3& diffuseColor,
		const Vec3& emissiveColor,
		const Vec3& specularColor,
		const Vec3& transparentColor,
		float specularReflection=0,
		float diffuseReflection=1.0,
		float subsurfaceScattering=0,
		float refractionIndex=0)
	{
	};

	Vec3 diffuseColor;
	Vec3 emissiveColor;
	Vec3 specularColor;
	Vec3 transparentColor;
	float specularReflection;
	float diffuseReflection;
	float subsurfaceScattering;
	float refractionIndex;
};
