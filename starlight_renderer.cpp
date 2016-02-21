#include "starlight_renderer.h"
#include "starlight_platform.h"
#include <d3d11.h>
#include <imgui.h> // for the clear color
#include <algorithm>

#include <array>
#include "starlight_log.h"

#define MAX_DRAW_COMMANDS (1 << 16)

// Direct3D 11 device
static ID3D11Device* s_device;
static ID3D11DeviceContext* s_deviceContext;
static IDXGISwapChain* s_swapChain;
static ID3D11RenderTargetView* s_renderTargetView;
static ID3D11DepthStencilState* s_depthStencilState;
static ID3D11DepthStencilView* s_depthStencilView;
static ID3D11Texture2D* s_depthStencilBuffer;

static std::array<renderer::DrawCommand, MAX_DRAW_COMMANDS> s_drawCommands;
static int32_t s_numDrawCommands;

static ID3D11Buffer* s_constantBuffers[renderer::NumConstantBuffers];

static PerFrame* s_perFrame;
static PerCamera* s_perCamera;

void CreateRenderTarget();


ID3D11PixelShader* renderer::CreatePixelShader(const void *ptr, std::size_t size)
{
	ID3D11PixelShader* shader = nullptr;
	D3D_TRY(s_device->CreatePixelShader(ptr, size, nullptr, &shader));
	return shader;
}

ID3D11VertexShader* renderer::CreateVertexShader(const void *ptr, std::size_t size)
{
	ID3D11VertexShader* shader = nullptr;
	D3D_TRY(s_device->CreateVertexShader(ptr, size, nullptr, &shader));
	return shader;
}

ID3D11Device* renderer::GetDevice()
{
	return s_device;
}

ID3D11DeviceContext* renderer::GetDeviceContext()
{
	return s_deviceContext;
}

ID3D11RenderTargetView*& renderer::GetRenderTargetView()
{
	return s_renderTargetView;
}

ID3D11DepthStencilState* renderer::GetDepthStencilState()
{
	return s_depthStencilState;
}

ID3D11DepthStencilView* renderer::GetDepthStencilView()
{
	return s_depthStencilView;
}

void renderer::Submit()
{
	// Sort draw commands
	std::sort(s_drawCommands.begin(), s_drawCommands.begin() + s_numDrawCommands,
		[](renderer::DrawCommand i, renderer::DrawCommand j) {
		return i.key < j.key;
	}
	);

	// Gets screen size
	auto windowSize = platform::GetWindowSize();

	// Constant Buffers
	if (windowSize.x > 0 && windowSize.y > 0) {
		glm::mat4 projectionMatrix = glm::perspectiveFovLH(glm::radians(45.0f), (float)windowSize.x, (float)windowSize.y, 0.1f, 100.0f);
		s_deviceContext->UpdateSubresource(s_constantBuffers[Camera], 0, nullptr, &projectionMatrix, 0, 0);
	}

	s_deviceContext->UpdateSubresource(s_constantBuffers[Frame], 0, nullptr, s_perCamera, 0, 0);

	// Submit draw commands
	for (int32_t i = 0; i < s_numDrawCommands; i++) {
		DrawCommand& cmd = s_drawCommands[i];
		PipelineState pipelineState = cmd.pipelineState;

		// Constant Buffers
		s_deviceContext->UpdateSubresource(s_constantBuffers[Object], 0, nullptr, cmd.perObject, 0, 0);

		// Input Assembler
		uint32_t stride = sizeof(Vertex);
		uint32_t offset = 0;
		s_deviceContext->IASetInputLayout(pipelineState.inputLayout);
		s_deviceContext->IASetVertexBuffers(0, 1, &cmd.mesh->vertexBuffer, &stride, &offset);
		s_deviceContext->IASetIndexBuffer(cmd.mesh->indexBuffer, DXGI_FORMAT_R16_UINT, 0);
		s_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Vertex Shader
		s_deviceContext->VSSetShader(pipelineState.vertexShader, nullptr, 0);
		s_deviceContext->VSSetConstantBuffers(0, NumConstantBuffers, s_constantBuffers);

		// TODO: Hull Shader

		// TODO: Domain Shader

		// TODO: Geometry Shader

		// Rasterizer
		s_deviceContext->RSSetState(pipelineState.rasterizerState);
		s_deviceContext->RSSetViewports(pipelineState.numViewports, pipelineState.viewports);

		// Pixel Shader
		s_deviceContext->PSSetShader(pipelineState.pixelShader, nullptr, 0);

		// Output Merger
		s_deviceContext->OMSetRenderTargets(1, &s_renderTargetView, nullptr);
		s_deviceContext->OMSetDepthStencilState(s_depthStencilState, 1);

		// Draw call
		s_deviceContext->DrawIndexed(cmd.mesh->numIndices, 0, 0);
	}
}

void renderer::SwapBuffers() {
	s_swapChain->Present(0, 0);
}

void renderer::Destroy()
{
	SafeRelease(s_device);
	SafeRelease(s_deviceContext);
	SafeRelease(s_swapChain);
	SafeRelease(s_renderTargetView);
	SafeRelease(s_depthStencilState);
	SafeRelease(s_depthStencilView);
	SafeRelease(s_depthStencilBuffer);
	for (int32_t i = 0; i < NumConstantBuffers; i++) {
		SafeRelease(s_constantBuffers[i]);
	}
}

void renderer::Clear()
{
	s_numDrawCommands = 0;
	ZeroMemory(&s_drawCommands, sizeof(s_drawCommands));
	ImVec4 clear_col = ImColor(114, 144, 154);
	s_deviceContext->ClearRenderTargetView(s_renderTargetView, (float*)&clear_col);
	s_deviceContext->ClearDepthStencilView(s_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void renderer::Resize()
{
	SafeRelease(s_renderTargetView);
	D3D_TRY(s_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
	CreateRenderTarget();
}

void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	s_swapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D_TRY(s_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer));
	D3D_TRY(s_device->CreateRenderTargetView(pBackBuffer, nullptr, &s_renderTargetView));
	s_deviceContext->OMSetRenderTargets(1, &s_renderTargetView, nullptr);
	pBackBuffer->Release();
}

HRESULT renderer::Init()
{
	auto screenSize = platform::GetWindowSize();
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	{
		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDesc.BufferCount = 1; // TODO: 2?
		swapChainDesc.BufferDesc.Width = screenSize.x;
		swapChainDesc.BufferDesc.Height = screenSize.y;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60; // FIXME: Hardcoded!
															 //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
															 //swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = platform::GetHWND();
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Windowed = TRUE;
	}

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	D3D_TRY(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&s_swapChain,
		&s_device,
		&featureLevel,
		&s_deviceContext));

	CreateRenderTarget();

	// Depth Stencil
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	auto windowSize = platform::GetWindowSize();

	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = windowSize.x;
	depthStencilBufferDesc.Height = windowSize.y;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D_TRY(renderer::GetDevice()->CreateTexture2D(&depthStencilBufferDesc, nullptr, &s_depthStencilBuffer));

	D3D_TRY(renderer::GetDevice()->CreateDepthStencilView(s_depthStencilBuffer, nullptr, &s_depthStencilView));

	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(depthStencilStateDesc));
	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;
	D3D_TRY(renderer::GetDevice()->CreateDepthStencilState(&depthStencilStateDesc, &s_depthStencilState));

	// Constant buffer descriptor
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	// Per frame constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerFrame);
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[Frame]));

	// Per camera constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerCamera);
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[Camera]));

	// Per object constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerObject);
	D3D_TRY(renderer::GetDevice()->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[Object]));

	return S_OK;
}

uint64_t renderer::encodeDraw(renderer::SortKey1 sortkey)
{
	// |               3               2               1               0|
	// |fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210|
	// | vvvvvvvvdsssssssssssttpppppppppdddddddddddddddddddddddddddddddd|
	// |        ^^          ^ ^        ^                               ^|
	// |        ||          | |        |                               ||
	// |   view-+|      seq-+ +-trans  +-program                 depth-+|
	// |         +-draw                                                 |

	uint64_t depth = (uint64_t(sortkey.depth) << 0x00) & 0xFFFFFFFF;
	uint64_t program = (uint64_t(sortkey.program) << 0x20) & 0x01FF;
	uint64_t trans = (uint64_t(sortkey.trans) << 0x29) & 0x03;
	uint64_t seq = (uint64_t(sortkey.seq) << 0x2B) & 0x07FF;
	uint64_t view = (uint64_t(sortkey.view) << 0x37) & 0xFF;

	uint64_t key = depth | program | trans | (1ULL << 0x36) | seq | view;

	//BX_CHECK(seq == (uint64_t(m_seq) << SORT_KEY_SEQ_SHIFT), "SortKey error, sequence is truncated (m_seq: %d).", m_seq);

	return key;
}

renderer::SortKey1 renderer::decode(uint64_t _key)
{
	SortKey1 key;
	key.seq = uint32_t((_key & 0x07FF) >> 0x2B);
	key.view = uint8_t((_key & 0xFF) >> 0x37);
	key.depth = uint32_t((_key & 0xFFFFFFFF) >> 0x00);
	key.program = uint16_t((_key & 0x01FF) >> 0x20);
	key.trans = uint8_t((_key & 0x03) >> 0x29);
	return key;
}

void renderer::AddDrawCommand(DrawCommand pair) {
	s_drawCommands[s_numDrawCommands] = pair;
	s_numDrawCommands++;
}

void renderer::SetPerFrame(PerFrame* perFrame) {
	s_perFrame = perFrame;
}

void renderer::SetPerCamera(PerCamera* perCamera) {
	s_perCamera = perCamera;
}
