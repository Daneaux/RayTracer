struct VSOut {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOut main(uint vertexID : SV_VertexID) {
    VSOut o;
    o.uv  = float2((vertexID << 1) & 2, vertexID & 2);
    o.pos = float4(o.uv.x * 2.0f - 1.0f, -(o.uv.y * 2.0f - 1.0f), 0.0f, 1.0f);
    return o;
}
