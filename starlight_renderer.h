#pragma once
#include <cstddef>
#include <Windows.h>
#include "starlight_glm.h"
#include "starlight_cbuffer.h"

#ifdef _DEBUG
#include <sstream>
#define STR(x) #x
#define XSTR(x) STR(x)
#define D3D_TRY(expr) \
do { \
	HRESULT	hr = expr; \
	if (FAILED( hr )) { \
		std::stringstream s;\
		s << __FILE__ << "(" << __LINE__ << "): " << STR(expr) << "failed\n";\
		logger::LogInfo(s.str());\
		_CrtDbgBreak(); \
	} \
} while (0)
#else
#define D3D_TRY(expr) expr
#endif

template<typename T>
inline void SafeRelease(T& ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
};

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

// TODO: Base vertex location, start index (assumed zero)
struct Mesh {
	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;
	int32_t numIndices = -1;
};

namespace renderer {

	enum EConstantBuffer
	{
		Frame,
		Camera,
		Object,
		//Material,
		NumConstantBuffers
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

	struct DrawCommand {
		uint64_t key;
		Mesh* mesh;
		PipelineState pipelineState;
		//PerFrame* perFrame;
		//PerCamera* perCamera;
		PerObject* perObject;
	};

	ID3D11PixelShader* CreatePixelShader(const void* ptr, std::size_t size);
	ID3D11VertexShader* CreateVertexShader(const void* ptr, std::size_t size);

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();
	ID3D11RenderTargetView*& GetRenderTargetView();
	ID3D11DepthStencilState* GetDepthStencilState();
	ID3D11DepthStencilView* GetDepthStencilView();

	void AddDrawCommand(DrawCommand pair);
	void SetPerFrame(PerFrame* perFrame);
	void SetPerCamera(PerCamera* perCamera);
	void Submit();
	void SwapBuffers();

	HRESULT Init();
	void Destroy();
	void Clear();
	void Resize();
}
