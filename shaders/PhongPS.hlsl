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
    // ============================================================
    // INNER LOOP (GPU): This shader runs once per pixel covered
    // by the sphere mesh. To paint a pixel, return a float4(R,G,B,A).
    //
    // --- SAMPLE: paint a pixel directly ---
    // Example 1: Solid magenta
    //   return float4(1.0, 0.0, 1.0, 1.0);
    //
    // Example 2: Visualize world-space normals as color
    //   float3 N = normalize(input.worldNorm);
    //   return float4(N * 0.5 + 0.5, 1.0);
    //
    // Example 3: Screen-space checkerboard on the sphere
    //   int2 screenPos = (int2)input.posH.xy;
    //   float checker = ((screenPos.x / 8 + screenPos.y / 8) % 2) ? 1.0 : 0.2;
    //   return float4(checker, checker * 0.5, 0.0, 1.0);
    // ============================================================

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
