// This file is generated by hlsl_codegen.lua

#ifdef __cplusplus

#include "starlight_glm.h"
#include <d3d11.h>

D3D11_INPUT_ELEMENT_DESC g_AppData[] =
{
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct AppData
{
	glm::vec2 uv;
	glm::vec3 position;
};

struct CBView
{
	glm::mat4 view;
};

struct CBProjection
{
	glm::mat4 projection;
};

struct CBModel
{
	glm::mat4 worldMatrix;
};

#else

struct AppData
{
	float2 uv : TEXCOORD;
	float3 position : POSITION;
};

cbuffer CBView : register(b0)
{
	matrix view;
};

cbuffer CBProjection : register(b1)
{
	matrix projection;
};

cbuffer CBModel : register(b2)
{
	matrix worldMatrix;
};

#endif