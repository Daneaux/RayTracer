Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    return g_texture.Sample(g_sampler, uv);
}
