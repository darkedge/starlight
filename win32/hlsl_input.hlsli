struct PS_INPUT_WIRE // new
{
    float4 Pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    noperspective float4 EdgeA: TEXCOORD1;
    noperspective float4 EdgeB: TEXCOORD2;
    uint Case : TEXCOORD3;
};

struct GS_INPUT
{
	float4 position : POSITION;
    float2 uv : TEXCOORD0;
};


#if 0
struct PS_INPUT_WIRE
{
    float4 Pos : SV_POSITION;
    float4 Col : TEXCOORD0;
    noperspective float4 EdgeA: TEXCOORD1;
    noperspective float4 EdgeB: TEXCOORD2;
    uint Case : TEXCOORD3;
};

struct GS_INPUT
{
    float4 Pos  : POSITION;
    float4 PosV : TEXCOORD0;
};
#endif
