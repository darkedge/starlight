#pragma once 
#include "starlight_graphics.h"
#include "starlight_renderer_windows.h"

struct ID3D11PixelShader;
struct ID3D11VertexShader;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11SamplerState;
struct D3D11_VIEWPORT;
struct ID3D11RasterizerState;

struct PipelineState {
	ID3D11InputLayout* inputLayout;
	//ID3D11SamplerState* samplerState;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	//ID3D11Buffer** constantBuffers;
	D3D11_VIEWPORT* viewports;
	ID3D11RasterizerState* rasterizerState;
	int32_t numViewports;
	//int32_t numConstantBuffers;

	// TODO: Textures?
	//int32_t id;
};

struct EConstantBuffer {
	enum
	{
		Model,
		View,
		Projection,
		Count,
	};
};

struct MeshD3D11; // TODO: Put definition here?

struct DrawCommand {
	union {
		struct {
			uint64_t key;
			MeshD3D11* mesh;
			//Mesh* mesh;
			PipelineState pipelineState;
			//PerFrame* perFrame;
			//PerCamera* perCamera;
			//Vectormath::Aos::Matrix4 worldMatrix;
			float worldMatrix[16];
		} live;
		DrawCommand* next;
	} state;
};

struct SortKey1 {
	uint32_t depth;
	uint32_t seq;
	uint16_t program;
	uint8_t  view;
	uint8_t  trans;
};

// depth: 32 bits, offset 0x00
// program: 9 bits, offset 0x20
// trans: 2 bits, offset 0x29
// seq: 11 bits, offset 0x2b
// draw: 1 bit, offset 0x36
// view: 8 bits, offset 0x37
uint64_t encodeDraw(SortKey1 sortkey);

SortKey1 decode(uint64_t _key);