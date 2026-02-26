float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    return float4(uv.x, uv.y, 0.5f, 1.0f);
}
