struct PixelShaderInput
{
	float2 uv : TEXCOORD;
	float4 position : SV_POSITION;
};

float4 main( PixelShaderInput IN ) : SV_TARGET
{
    return float4(IN.uv, 1.0f, 1.0f);
}
