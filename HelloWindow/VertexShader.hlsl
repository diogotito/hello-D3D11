#include "ShaderCommon.hlsli"

float4 main( float4 pos : POSITION, /* TODO fix this: uint vertexId : SV_VertexID,*/ out float4 oPos : POSITION ) : SV_Position
{
    static const float freq = 2.0f;
    float foo2 = 1.1f - 0.3f * pow(2.0f * (time * freq % 1.0f) - 1, 2.0f);
    float2 newPos = pos.xy * foo2;
    oPos =  float4(newPos, pos.z, 1.0f);
    return oPos;
}