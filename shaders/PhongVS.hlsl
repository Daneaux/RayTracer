cbuffer CBPerObject : register(b0) {
    float4x4 World;
    float4x4 WorldViewProj;
    float4x4 WorldInverseTranspose;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct VSOutput {
    float4 posH      : SV_Position;
    float3 worldPos  : TEXCOORD0;
    float3 worldNorm : TEXCOORD1;
};

VSOutput main(VSInput input) {
    VSOutput o;
    o.posH      = mul(float4(input.position, 1.0f), WorldViewProj);
    o.worldPos  = mul(float4(input.position, 1.0f), World).xyz;
    o.worldNorm = mul(float4(input.normal,   0.0f), WorldInverseTranspose).xyz;
    return o;
}
