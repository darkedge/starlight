#include "starlight_hlsl_generated.h"

struct VertexShaderOutput
{
    float2 uv : TEXCOORD;
    float4 position : SV_POSITION;
};

VertexShaderOutput main( Vertex IN )
{
    VertexShaderOutput OUT;

    matrix mvp = mul( projection, mul( view, worldMatrix ) );
    OUT.position = mul( mvp, float4( IN.position, 1.0f ) );
    OUT.uv = IN.uv;

    return OUT;
}
