struct PixelShaderInput
{
	float3 uv : TEXCOORD;
	float4 position : SV_POSITION;
};

Texture2DArray Texture : register(t0);
sampler Sampler : register(s0);

float4 main( PixelShaderInput IN ) : SV_TARGET
{
	return Texture.Sample(Sampler, IN.uv);
}
