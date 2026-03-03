cbuffer CBPerObject : register(b0) {
    float4x4 WorldViewProj;
};

struct VSInput {
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct VSOutput {
    float4 posH  : SV_Position;
    float4 color : COLOR;
};

VSOutput main(VSInput input) {
    VSOutput o;
    o.posH  = mul(float4(input.position, 1.0f), WorldViewProj);
    o.color = input.color;
    return o;
}
