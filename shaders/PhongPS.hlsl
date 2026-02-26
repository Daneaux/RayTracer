cbuffer CBPerFrame : register(b1) {
    float4 LightPosition;    // .xyz = position
    float4 LightColor;       // .xyz = color, .w = intensity
    float4 CameraPosition;   // .xyz = eye position
    float4 AmbientColor;     // .xyz = ambient
    float4 SphereColor;      // .xyz = diffuse color, .w = specular power
};

struct PSInput {
    float4 posH      : SV_Position;
    float3 worldPos  : TEXCOORD0;
    float3 worldNorm : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target {
    float3 N = normalize(input.worldNorm);
    float3 L = normalize(LightPosition.xyz - input.worldPos);
    float3 V = normalize(CameraPosition.xyz - input.worldPos);
    float3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0f);
    float spec = pow(max(dot(N, H), 0.0f), SphereColor.w);

    float3 ambient  = AmbientColor.xyz * SphereColor.xyz;
    float3 diffuse  = diff * SphereColor.xyz * LightColor.xyz * LightColor.w;
    float3 specular = spec * LightColor.xyz * LightColor.w;

    float3 finalColor = ambient + diffuse + specular;
    return float4(saturate(finalColor), 1.0f);
}
