float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    // ============================================================
    // INNER LOOP (GPU): This shader runs once per pixel of the
    // fullscreen background quad. pos.xy = screen pixel coords,
    // uv = normalized 0..1 coordinates. Return float4(R,G,B,A).
    //
    // --- SAMPLE: paint a pixel directly ---
    // Example 1: Solid dark blue background
    //   return float4(0.05, 0.05, 0.2, 1.0);
    //
    // Example 2: Screen-space radial gradient
    //   float2 center = float2(0.5, 0.5);
    //   float dist = length(uv - center);
    //   return float4(dist, 0.2, 1.0 - dist, 1.0);
    //
    // Example 3: Pixel-level XOR pattern
    //   int2 p = (int2)pos.xy;
    //   float xorVal = ((p.x ^ p.y) & 0xFF) / 255.0;
    //   return float4(xorVal, xorVal, xorVal, 1.0);
    // ============================================================

    return float4(uv.x, uv.y, 0.5f, 1.0f);
}
