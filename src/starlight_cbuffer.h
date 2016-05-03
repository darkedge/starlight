#ifdef __cplusplus
// C++
#pragma once
#include "starlight_glm.h"
#include <d3d11.h>

#define CBUFFER(a,b) struct a
#define INPUT_NAME(x) D3D11_INPUT_ELEMENT_DESC g_##x[] =
#define VEC(i, x, y) { #y, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
typedef glm::mat4 matrix;

#else
// HLSL
#define CBUFFER(a,b) cbuffer a : register(b)
#define INPUT_NAME(x) struct x
#define VEC(i, x, y) float##i x : y;
#endif

INPUT_NAME(AppData)
{
	VEC(2, uv, TEXCOORD)
	VEC(3, position, POSITION)
};

CBUFFER(CBView, b0)
{
	matrix view;
};

CBUFFER(CBProjection, b1)
{
	matrix projection;
};

CBUFFER(CBModel, b2)
{
	matrix worldMatrix;
};
