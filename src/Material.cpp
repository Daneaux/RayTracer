#include "Material.h"
#include "math/MathTypes.h"

float Material::calculate_pdf(Vec3 normal, Vec3 view_dir, Vec3 light_dir) {
	// 1. Calculate the 'Selection Probabilities'
	// Usually based on your metallic/specular weights
	float diffuse_chance = (1.0f - metallic);
	float specular_chance = 1.0f - diffuse_chance;

	// 2. Diffuse PDF (Cosine weighted)
	float cos_theta = std::max(0.0f, Vec3::Dot(normal, light_dir));
	float pdf_diffuse = cos_theta / PI;

	// 3. Specular PDF (GGX Importance Sampling)
	Vec3 half_vector = (view_dir + light_dir).Normalized();
	float Dot_nh = std::max(0.0f, Vec3::Dot(normal, half_vector));
	float Dot_vh = std::max(0.0f, Vec3::Dot(view_dir, half_vector));

	// D = GGX Distribution function
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = (Dot_nh * Dot_nh * (alpha2 - 1.0f) + 1.0f);
	float D = alpha2 / (PI * denom * denom);

	float pdf_specular = (D * Dot_nh) / (4.0f * Dot_vh);

	// 4. Final Weighted PDF
	return (diffuse_chance * pdf_diffuse) + (specular_chance * pdf_specular);
}