#include "starlight_cbuffer.h"

struct AppData
{
    float3 position : POSITION;
    float3 color: COLOR;
};

struct VertexShaderOutput
{
    float4 color : COLOR;
    float4 position : SV_POSITION;
};

VertexShaderOutput main( AppData IN )
{
    VertexShaderOutput OUT;

    matrix mvp = mul( projection, mul( view, worldMatrix ) );
    OUT.position = mul( mvp, float4( IN.position, 1.0f ) );
    OUT.color = float4( IN.color, 1.0f );

    return OUT;
}
