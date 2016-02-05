cbuffer PerApplication : register(b0)
{
	matrix projectionMatrix;
}//4 * 16 bytes total, 64 bytes

cbuffer PerFrame : register(b1)
{
	matrix viewMatrix;
}//4 * 16 bytes total, 64 bytes

cbuffer PerObject : register(b2)
{
	matrix worldMatrix;
}//4 * 16 bytes total, 64 bytes

struct AppData
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VertexShaderOutput
{
	float4 color : COLOR;
	float4 position : SV_POSITION;
};

VertexShaderOutput main(AppData IN)
{
	VertexShaderOutput OUT;

	matrix mvp = mul(projectionMatrix, viewMatrix);
	OUT.position = mul(mvp, float4(IN.position, 1));
	OUT.color = IN.color;

	return OUT;
}
