#include "ShaderCommon.hlsli"

float4 main( float4 pos : SV_Position, in float4 iPos : POSITION ) : SV_Target
{
    static const float freq = 2.0f;
    float foo1 = 1.1f - 0.3f * abs(sin(1.0f * PI * freq * time));
    float foo2 = 0.1f + 0.8f * (iPos.x + iPos.y);
    return float4(1.0f - foo2, foo1, foo2, 1.0f);
}