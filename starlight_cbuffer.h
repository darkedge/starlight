#ifdef __cplusplus
#pragma once
#include "starlight_glm.h"
#define CBUFFER(a,b) struct a
#define matrix glm::mat4
#else
#define CBUFFER(a,b) cbuffer a : register(b)
#endif

CBUFFER(PerCamera, b0)
{
	matrix view;
};

CBUFFER(PerFrame, b1)
{
	matrix projection;
};

CBUFFER(PerObject, b2)
{
	matrix worldMatrix;
};
