// Generated by hlsl_codegen.lua

#ifdef __cplusplus

#include <vectormath/scalar/cpp/vectormath_aos.h>
#include <d3d11.h>

D3D11_INPUT_ELEMENT_DESC g_Vertex[] =
{
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct CBModel
{
	Vectormath::Aos::Matrix4 worldMatrix;
};

struct CBView
{
	Vectormath::Aos::Matrix4 view;
};

struct CBProjection
{
	Vectormath::Aos::Matrix4 projection;
};

#else

struct Vertex
{
	float2 uv : TEXCOORD;
	float3 position : POSITION;
};

cbuffer CBModel : register(b0)
{
	matrix worldMatrix;
};

cbuffer CBView : register(b1)
{
	matrix view;
};

cbuffer CBProjection : register(b2)
{
	matrix projection;
};

#endif