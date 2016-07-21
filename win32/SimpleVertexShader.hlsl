#include "starlight_hlsl_generated.h"
#include "hlsl_input.hlsli"

GS_INPUT main(Vertex IN) {
    GS_INPUT output;

	// TODO: put in constant buffer
    matrix mvp = mul(projection, mul(view, worldMatrix));
    //matrix mv = mul(projection, view);

    output.position = mul(mvp, float4(IN.position, 1.0f));
    //output.PosV = mul(mv, float4(IN.position, 1.0f));
    output.uv = float2(IN.uv.x, 1.0f - IN.uv.y);

    return output;
}
