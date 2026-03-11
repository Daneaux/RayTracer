#pragma once
#include <math/MathTypes.h>


// Future: https://math.hws.edu/graphicsbook/c8/s2.html

struct Material_ {

	Material_() = default;
	Material_(
		const Vec3& diffuseColor,
		const Vec3& emissiveColor,
		const Vec3& specularColor,
		const Vec3& transparentColor,

		float transmission = 0,
		float specularReflection=0,
		float diffuseReflection=1.0,
		float subsurfaceScattering=0,
		float refractionIndex=0)
	{
		assert(diffuseReflection + specularReflection <= 1.0000001);
	};

	Vec3 diffuseColor;
	Vec3 emissiveColor;
	Vec3 specularColor;
	Vec3 transparentColor;

	float transmission;

	float specularReflection;

	// 1.0 diffuse means surface scatters non specular light evenly in all directions
	// 0.0 diffuseReflection means no diffuse light response, totally black unless lit by reflections. perfect mirror for example.
	float diffuseReflection;

	float subsurfaceScattering;
	float refractionIndex;
};


struct Material {
	// 1. BASE COLOR (Albedo)
	// For dielectrics: the diffuse color.
	// For metals: the specular reflectance color.
	Vec3 baseColor = Vec3(0.0f,0.0f,0.0f);

	// 2. SURFACE PROPERTIES
	// 0.0 = Smooth/Mirror, 1.0 = Matte/Rough.
	float roughness = 0.0f;
	// 0.0 = Plastic/Glass (Dielectric), 1.0 = Pure Metal.
	float metallic = 0.0f;

	// 3. OPTICAL PROPERTIES
	// Used for Fresnel reflection and Refraction paths.
	float ior = 1.0f;           // Index of Refraction (e.g., 1.5 for glass)
	float transmission = 0.0f;  // 0 = Opaque, 1 = Fully transparent (glass)

	// 4. EMISSION
	Vec3 emissive = Vec3(0.0f, 0.0f, 0.0f);       // Light the object actually glows with
	float emissivePower = 0.0f; // Multiplier for the glow intensity

	// 5. ADVANCED (Optional)
	float subsurface = 0.0f;    // For skin, wax, or marble
	Vec3 clearcoat = Vec3(0.0f, 0.0f, 0.0f);      // A secondary "lacquer" layer on top

	float calculate_pdf(Vec3 normal, Vec3 view_dir, Vec3 light_dir);
};




