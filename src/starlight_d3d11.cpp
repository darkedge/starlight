#include "starlight_graphics.h"
#if defined(_WIN32) && defined(STARLIGHT_D3D11)

#include "starlight_d3d_shared.h"
#include "starlight_d3d11.h"
#include "starlight_renderer_windows.h"

#include <D3Dcommon.h>

#include <algorithm>

#include <array>
#include "starlight_log.h"
#include "starlight_game.h"

#include "starlight_hlsl_generated.h"
#include "WICTextureLoader.h"

using namespace Vectormath::Aos;

struct MeshD3D11 {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int32_t numIndices;
};

#define MAX_DRAW_COMMANDS 1024
static DrawCommand g_drawCommands[MAX_DRAW_COMMANDS];
static int32_t g_numDrawCommands;

#define MAX_MESHES 1024
static MeshD3D11 g_meshes[MAX_MESHES];
static int32_t g_numMeshes;

// shaders (generated)
// changed to defines to prevent visual studio hanging
#include "SimplePixelShader.h"
#define PixelShaderBlob g_SimplePixelShader
#include "SimpleVertexShader.h"
#define VertexShaderBlob g_SimpleVertexShader

static ID3D11PixelShader* s_pixelShader;
static ID3D11VertexShader* s_vertexShader;
static ID3D11Buffer* s_constantBuffers[EConstantBuffer::Count];
static ID3D11InputLayout* s_inputLayout;
static ID3D11RasterizerState* s_rasterizerState;
static ID3D11DepthStencilState* s_depthStencilState;
static ID3D11DepthStencilView* s_depthStencilView;
static ID3D11Texture2D* s_depthStencilBuffer;
static ID3D11SamplerState* s_samplerState;
static ID3D11ShaderResourceView* s_shaderResourceView;
static ID3D11Resource* s_textureResource;
static D3D11_VIEWPORT s_viewport;

static Matrix4 s_view;
static Matrix4 s_projection;

#if 0



static D3D11_VIEWPORT s_viewport;


ID3D11PixelShader* CreatePixelShader(const void* ptr, std::size_t size);
ID3D11VertexShader* CreateVertexShader(const void* ptr, std::size_t size);

//void AddDrawCommand(DrawCommand drawCommand);
//void SetPerFrame(PerFrame* perFrame);
//void SetPerCamera(PerCamera* perCamera);

#define MAX_DRAW_COMMANDS (1 << 16)

// Direct3D 11 device
static ID3D11Device* s_device;
static ID3D11DeviceContext* s_deviceContext;
static IDXGISwapChain* s_swapChain;
static ID3D11RenderTargetView* s_renderTargetView;
static ID3D11DepthStencilState* s_depthStencilState;
static ID3D11DepthStencilView* s_depthStencilView;
static ID3D11Texture2D* s_depthStencilBuffer;

//static std::array<d3d11::DrawCommand, MAX_DRAW_COMMANDS> s_drawCommands;
//static int32_t s_numDrawCommands;

static ID3D11Buffer* s_constantBuffers[NumConstantBuffers];

//static PerFrame* s_perFrame;
//static PerCamera* s_perCamera;

ID3D11PixelShader* CreatePixelShader(const void *ptr, std::size_t size)
{
	ID3D11PixelShader* shader = nullptr;
	D3D_TRY(s_device->CreatePixelShader(ptr, size, nullptr, &shader));
	return shader;
}

ID3D11VertexShader* CreateVertexShader(const void *ptr, std::size_t size)
{
	ID3D11VertexShader* shader = nullptr;
	D3D_TRY(s_device->CreateVertexShader(ptr, size, nullptr, &shader));
	return shader;
}

void graphics::D3D11::Render() {
	//s_numDrawCommands = 0;
	//ZeroMemory(&s_drawCommands, sizeof(s_drawCommands));

	// Clear
	ImVec4 clear_col = ImColor(114, 144, 154);
	s_deviceContext->ClearRenderTargetView(s_renderTargetView, (float*) &clear_col);
	s_deviceContext->ClearDepthStencilView(s_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

#if 0
	renderer::DrawCommand cmd;
	ZeroMemory(&cmd, sizeof(cmd));

	cmd.mesh = &s_mesh;
	cmd.pipelineState.inputLayout = s_inputLayout;
	cmd.pipelineState.numViewports = 1;
	cmd.pipelineState.viewports = &s_viewport;
	cmd.pipelineState.pixelShader = s_pixelShader;
	cmd.pipelineState.vertexShader = s_vertexShader;
	cmd.pipelineState.rasterizerState = s_rasterizerState;
	cmd.perObject = &s_perObject;

	renderer::AddDrawCommand(cmd);

	// now we should do the actual rendering

	// Sort draw commands
	std::sort(s_drawCommands.begin(), s_drawCommands.begin() + s_numDrawCommands,
		[](d3d11::DrawCommand i, d3d11::DrawCommand j) {
		return i.key < j.key;
	}
	);

	// For retrieving the window size
	DXGI_SWAP_CHAIN_DESC desc;
	s_swapChain->GetDesc(&desc);

	// Constant Buffers
	if (desc.BufferDesc.Width > 0 && desc.BufferDesc.Height > 0) {
		glm::mat4 projectionMatrix = glm::perspectiveFovLH(glm::radians(45.0f), (float) windowSize.x, (float) windowSize.y, 0.1f, 100.0f);
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
#endif

	ImGui::Render();

	// Present
	s_swapChain->Present(0, 0);
}


void graphics::D3D11::Destroy()
{
	ImGui_ImplDX11_Shutdown();
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

	// from game
	SafeRelease(s_rasterizerState);
	SafeRelease(s_pixelShader);
	SafeRelease(s_vertexShader);
	SafeRelease(s_inputLayout);
	//SafeRelease(s_mesh.indexBuffer);
	//SafeRelease(s_mesh.vertexBuffer);
}

static void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	s_swapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D_TRY(s_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &pBackBuffer));
	D3D_TRY(s_device->CreateRenderTargetView(pBackBuffer, nullptr, &s_renderTargetView));
	s_deviceContext->OMSetRenderTargets(1, &s_renderTargetView, nullptr);
	pBackBuffer->Release();
}

void graphics::D3D11::Resize(int32_t width, int32_t height) {
	SafeRelease(s_renderTargetView);
	D3D_TRY(s_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
	CreateRenderTarget();
}

// TODO: Any error here is unrecoverable and should return false
bool graphics::D3D11::Init(PlatformData *data)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	{
		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDesc.BufferCount = 1; // TODO: 2?
		swapChainDesc.BufferDesc.Width = 0;
		swapChainDesc.BufferDesc.Height = 0;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60; // FIXME: Hardcoded!
															 //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
															 //swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = data->hWnd;
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

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
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
		&s_deviceContext);

	if (FAILED(hr)) {
		logger::LogInfo("Failed to initialize Direct3D 11 device!");
		return false;
	}

	CreateRenderTarget();

	// Depth Stencil
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	// For retrieving the window size
	DXGI_SWAP_CHAIN_DESC desc;
	s_swapChain->GetDesc(&desc);

	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = desc.BufferDesc.Width;
	depthStencilBufferDesc.Height = desc.BufferDesc.Height;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D_TRY(s_device->CreateTexture2D(&depthStencilBufferDesc, nullptr, &s_depthStencilBuffer));

	D3D_TRY(s_device->CreateDepthStencilView(s_depthStencilBuffer, nullptr, &s_depthStencilView));

	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(depthStencilStateDesc));
	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;
	D3D_TRY(s_device->CreateDepthStencilState(&depthStencilStateDesc, &s_depthStencilState));

	// Constant buffer descriptor
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

#if 0
	// Per frame constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerFrame);
	D3D_TRY(s_device->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[Frame]));

	// Per camera constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerCamera);
	D3D_TRY(s_device->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[Camera]));

	// Per object constant buffer
	constantBufferDesc.ByteWidth = sizeof(PerObject);
	D3D_TRY(s_device->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[Object]));
#endif
	
	// Rasterizer State
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	D3D_TRY(s_device->CreateRasterizerState(&rasterizerDesc, &s_rasterizerState));

	// Initialize the viewport to occupy the entire client area.
	s_viewport.Width = (float) desc.BufferDesc.Width;
	s_viewport.Height = (float) desc.BufferDesc.Height;
	s_viewport.TopLeftX = 0.0f;
	s_viewport.TopLeftY = 0.0f;
	s_viewport.MinDepth = 0.0f;
	s_viewport.MaxDepth = 1.0f;

	// Shaders
	s_pixelShader = CreatePixelShader(PixelShaderBlob, sizeof(PixelShaderBlob));
	s_vertexShader = CreateVertexShader(VertexShaderBlob, sizeof(VertexShaderBlob));

	// TODO Sampler (for texturing)


	// Input layout
	D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	D3D_TRY(s_device->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), VertexShaderBlob, sizeof(VertexShaderBlob), &s_inputLayout));

	ImGui_ImplDX11_Init(data->hWnd, s_device, s_deviceContext);

	{
		// Load Fonts (TODO: Hardcoded!)
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("external/imgui-1.49/extra_fonts/DroidSans.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	}

	return true;
}
#if 0
uint64_t d3d11::encodeDraw(d3d11::SortKey1 sortkey)
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

d3d11::SortKey1 d3d11::decode(uint64_t _key)
{
	SortKey1 key;
	key.seq = uint32_t((_key & 0x07FF) >> 0x2B);
	key.view = uint8_t((_key & 0xFF) >> 0x37);
	key.depth = uint32_t((_key & 0xFFFFFFFF) >> 0x00);
	key.program = uint16_t((_key & 0x01FF) >> 0x20);
	key.trans = uint8_t((_key & 0x03) >> 0x29);
	return key;
}

void d3d11::AddDrawCommand(DrawCommand pair) {
	s_drawCommands[s_numDrawCommands] = pair;
	s_numDrawCommands++;
}
#endif

#define MAX_MESHES 256
static MeshD3D11 g_meshes[MAX_MESHES];
static int32_t g_numMeshes;

int32_t graphics::D3D11::UploadMesh(Vertex* vertices, int32_t numVertices, int32_t* indices, int32_t numIndices) {
	return -1;
#if 0
	MeshD3D11 mesh;
	ZeroMemory(&mesh, sizeof(mesh));

	// vertex buffer
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };

		bufferDesc.ByteWidth = sizeof(vertices);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

		dataDesc.pSysMem = vertices;

		D3D_TRY(renderer::GetDevice()->CreateBuffer(&bufferDesc, &dataDesc, &mesh.vertexBuffer));
	}

	// index buffer
	{
		D3D11_BUFFER_DESC bufferDesc = { 0 };

		bufferDesc.ByteWidth = sizeof(indices);
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

		dataDesc.pSysMem = indices;

		D3D_TRY(renderer::GetDevice()->CreateBuffer(&bufferDesc, &dataDesc, &mesh.indexBuffer));
	}

	mesh.numIndices = _countof(indices);

	
	g_meshes[g_numMeshes] = mesh;
	auto t = g_numMeshes;
	g_numMeshes++;
	return t;
#endif
}

void graphics::D3D11::Update() {
	// TODO
}

void graphics::D3D11::ImGuiNewFrame() {
	ImGui_ImplDX11_NewFrame();
}

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool graphics::D3D11::ImGuiHandleEvent(WindowEvent* e) {
	if (ImGui_ImplDX11_WndProcHandler(e->hWnd, e->msg, e->wParam, e->lParam)) {
		return true;
	}
	return false;
}

void graphics::D3D11::SetPlayerCameraViewMatrix(glm::mat4 matrix) {
	// TODO
}

void graphics::D3D11::SetProjectionMatrix(glm::mat4 matrix) {
	// TODO
}

#endif

#include <imgui.h>
#include "examples\directx11_example\imgui_impl_dx11.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

// Data
// It is okay for this to be file/global.
// This stuff is volatile and can be reconstructed at any time.
static ID3D11Device* sl_pd3dDevice;
static ID3D11DeviceContext* sl_pd3dDeviceContext;
static IDXGISwapChain* g_pSwapChain;
static ID3D11RenderTargetView* g_mainRenderTargetView;

static void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	g_pSwapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer);
	sl_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
	sl_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	pBackBuffer->Release();

	// Depth Stencil
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = sd.BufferDesc.Width;
	depthStencilBufferDesc.Height = sd.BufferDesc.Height;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	sl_pd3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &s_depthStencilBuffer);

	sl_pd3dDevice->CreateDepthStencilView(s_depthStencilBuffer, nullptr, &s_depthStencilView);

	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;
	sl_pd3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &s_depthStencilState);

	// Initialize the viewport to occupy the entire client area.
	s_viewport.Width = (float) sd.BufferDesc.Width;
	s_viewport.Height = (float) sd.BufferDesc.Height;
	s_viewport.TopLeftX = 0.0f;
	s_viewport.TopLeftY = 0.0f;
	s_viewport.MinDepth = 0.0f;
	s_viewport.MaxDepth = 1.0f;
}

static void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

static HRESULT CreateDeviceD3D(HWND hWnd, GameFuncs* funcs)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	{
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	}

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	D3D_FEATURE_LEVEL featureLevelArray[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createDeviceFlags,
		featureLevelArray,
		_countof(featureLevelArray),
		D3D11_SDK_VERSION,
		&sd,
		&g_pSwapChain,
		&sl_pd3dDevice,
		&featureLevel,
		&sl_pd3dDeviceContext);
	if (hr != S_OK) {
		// Try without debug flag
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			0,
			featureLevelArray,
			_countof(featureLevelArray),
			D3D11_SDK_VERSION,
			&sd,
			&g_pSwapChain,
			&sl_pd3dDevice,
			&featureLevel,
			&sl_pd3dDeviceContext);
		if (hr != S_OK) {
			return E_FAIL;
		}
	}

	const char* level =
		featureLevel == D3D_FEATURE_LEVEL_11_1 ? "11.1" :
		featureLevel == D3D_FEATURE_LEVEL_11_0 ? "11.0" :
		featureLevel == D3D_FEATURE_LEVEL_10_1 ? "10.1" :
		featureLevel == D3D_FEATURE_LEVEL_10_0 ? "10.0" :
		featureLevel == D3D_FEATURE_LEVEL_9_3 ? "9.3" :
		featureLevel == D3D_FEATURE_LEVEL_9_2 ? "9.2" :
		featureLevel == D3D_FEATURE_LEVEL_9_1 ? "9.1" : "error";

	g_LogInfo(std::string("D3D Feature level: ") + level);
	

	// Setup rasterizer
	{
		D3D11_RASTERIZER_DESC RSDesc;
		memset(&RSDesc, 0, sizeof(D3D11_RASTERIZER_DESC));
		RSDesc.FillMode = D3D11_FILL_SOLID;
		RSDesc.CullMode = D3D11_CULL_NONE;
		RSDesc.FrontCounterClockwise = FALSE;
		RSDesc.DepthBias = 0;
		RSDesc.SlopeScaledDepthBias = 0.0f;
		RSDesc.DepthBiasClamp = 0;
		RSDesc.DepthClipEnable = TRUE;
		RSDesc.ScissorEnable = TRUE;
		RSDesc.AntialiasedLineEnable = FALSE;
		RSDesc.MultisampleEnable = (sd.SampleDesc.Count > 1) ? TRUE : FALSE;

		ID3D11RasterizerState* pRState = NULL;
		sl_pd3dDevice->CreateRasterizerState(&RSDesc, &pRState);
		sl_pd3dDeviceContext->RSSetState(pRState);
		pRState->Release();
	}

	CreateRenderTarget();

	return S_OK;
}

static void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (sl_pd3dDeviceContext) { sl_pd3dDeviceContext->Release(); sl_pd3dDeviceContext = NULL; }
	if (sl_pd3dDevice) { sl_pd3dDevice->Release(); sl_pd3dDevice = NULL; }
}

// graphics::API implementation

void graphics::D3D11::Destroy() {
	ImGui_ImplDX11_Shutdown();
	CleanupDeviceD3D();
}

void graphics::D3D11::Render() {
	// Clear
	ImVec4 clear_col = ImColor(114, 144, 154);
	sl_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*) &clear_col);
	sl_pd3dDeviceContext->ClearDepthStencilView(s_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//DrawCommand cmd;
	//ZeroMemory(&cmd, sizeof(cmd));

#if 0
	cmd.mesh = &s_mesh;
	cmd.pipelineState.inputLayout = s_inputLayout;
	cmd.pipelineState.numViewports = 1;
	cmd.pipelineState.viewports = &s_viewport;
	cmd.pipelineState.pixelShader = s_pixelShader;
	cmd.pipelineState.vertexShader = s_vertexShader;
	cmd.pipelineState.rasterizerState = s_rasterizerState;
	cmd.perObject = &s_perObject;

	renderer::AddDrawCommand(cmd);
#endif

	// now we should do the actual rendering

	// Bubble sort draw commands
	// Currently just contains a single chunk
	// No need to clear the buffer
	bool sorting = true;
	while (sorting) {
		sorting = false;
		for (int i = 1; i < g_numDrawCommands; i++) {
			if (g_drawCommands[i].key < g_drawCommands[i - 1].key ) {
				DrawCommand t = g_drawCommands[i];
				g_drawCommands[i] = g_drawCommands[i - 1];
				g_drawCommands[i - 1] = t;
				sorting = true;
			}
		}
	}

	// For retrieving the window size
	DXGI_SWAP_CHAIN_DESC desc;
	g_pSwapChain->GetDesc(&desc);

	// Constant Buffers
	// TODO: z-min, z-max (optional), FOV!
	if (desc.BufferDesc.Width > 0 && desc.BufferDesc.Height > 0) {
		Matrix4 projectionMatrix = Matrix4::perspective(45.0f * DEG2RAD, (float) desc.BufferDesc.Width / (float) desc.BufferDesc.Height, 0.1f, 100.0f);
		assert(s_constantBuffers[EConstantBuffer::Projection]);
		sl_pd3dDeviceContext->UpdateSubresource(s_constantBuffers[EConstantBuffer::Projection], 0, nullptr, &projectionMatrix, 0, 0);
	}

	assert(s_constantBuffers[EConstantBuffer::View]);
	sl_pd3dDeviceContext->UpdateSubresource(s_constantBuffers[EConstantBuffer::View], 0, nullptr, &s_view, 0, 0);

	// Submit draw commands
	for (int32_t i = 0; i < g_numDrawCommands; i++) {
		DrawCommand* cmd = &g_drawCommands[i];
		assert(cmd);

		MeshD3D11* mesh = &g_meshes[cmd->mesh];
		assert(mesh);
		PipelineState* pipelineState = &cmd->pipelineState;

		// Constant Buffers
		assert(s_constantBuffers[EConstantBuffer::Model]);
		sl_pd3dDeviceContext->UpdateSubresource(s_constantBuffers[EConstantBuffer::Model], 0, nullptr, &cmd->worldMatrix, 0, 0);

		// Input Assembler
		uint32_t stride = sizeof(Vertex);
		uint32_t offset = 0;
		//assert(pipelineState.inputLayout);
		//sl_pd3dDeviceContext->IASetInputLayout(pipelineState.inputLayout);
		sl_pd3dDeviceContext->IASetInputLayout(s_inputLayout);
		sl_pd3dDeviceContext->IASetVertexBuffers(0, 1, &mesh->vertexBuffer, &stride, &offset);
		sl_pd3dDeviceContext->IASetIndexBuffer(mesh->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		sl_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Vertex Shader
		sl_pd3dDeviceContext->VSSetShader(pipelineState->vertexShader, nullptr, 0);
		sl_pd3dDeviceContext->VSSetConstantBuffers(0, EConstantBuffer::Count, s_constantBuffers);

		// TODO: Hull Shader

		// TODO: Domain Shader

		// TODO: Geometry Shader

		// Rasterizer
		sl_pd3dDeviceContext->RSSetState(s_rasterizerState);
		sl_pd3dDeviceContext->RSSetViewports(1, &s_viewport);

		// Pixel Shader
		sl_pd3dDeviceContext->PSSetShader(pipelineState->pixelShader, nullptr, 0);
		sl_pd3dDeviceContext->PSSetSamplers( 0, 1, &s_samplerState );
		sl_pd3dDeviceContext->PSSetShaderResources( 0, 1, &s_shaderResourceView );

		// Output Merger
		sl_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, s_depthStencilView);
		sl_pd3dDeviceContext->OMSetDepthStencilState(s_depthStencilState, 1);

		// Draw call
		sl_pd3dDeviceContext->DrawIndexed(mesh->numIndices, 0, 0);
	}

	ImGui::Render();

	// Present
	g_pSwapChain->Present(0, 0);
}

void graphics::D3D11::Resize(int32_t, int32_t) {
	CleanupRenderTarget();
	g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTarget();
}

bool graphics::D3D11::Init(PlatformData *data, GameFuncs* funcs) {
	// Initialize Direct3D
	if (CreateDeviceD3D(data->hWnd, funcs) < 0)
	{
		CleanupDeviceD3D();
		return false;
	}

	D3D_TRY(sl_pd3dDevice->CreatePixelShader(PixelShaderBlob, sizeof(PixelShaderBlob), nullptr, &s_pixelShader));
	D3D_TRY(sl_pd3dDevice->CreateVertexShader(VertexShaderBlob, sizeof(VertexShaderBlob), nullptr, &s_vertexShader));
	D3D_TRY(sl_pd3dDevice->CreateInputLayout(g_Vertex, COUNT_OF(g_Vertex), VertexShaderBlob, sizeof(VertexShaderBlob), &s_inputLayout));

	// Create constant buffers

	// Constant buffer descriptor
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	constantBufferDesc.ByteWidth = sizeof(CBModel);
	D3D_TRY(sl_pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[EConstantBuffer::Model]));
	{
		const char c_szName [] = "CBModel";
		s_constantBuffers[EConstantBuffer::Model]->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
	}

	constantBufferDesc.ByteWidth = sizeof(CBView);
	D3D_TRY(sl_pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[EConstantBuffer::View]));
	{
		const char c_szName [] = "CBView";
		s_constantBuffers[EConstantBuffer::View]->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
	}

	constantBufferDesc.ByteWidth = sizeof(CBProjection);
	D3D_TRY(sl_pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &s_constantBuffers[EConstantBuffer::Projection]));
	{
		const char c_szName [] = "CBProjection";
		s_constantBuffers[EConstantBuffer::Projection]->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
	}

	// Rasterizer State
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	D3D_TRY(sl_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &s_rasterizerState));

	// Create a sampler state for texture sampling in the pixel shader
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof(D3D11_SAMPLER_DESC) );
	 
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;
	 
	D3D_TRY(sl_pd3dDevice->CreateSamplerState( &samplerDesc, &s_samplerState ));

	// Texture
	CreateWICTextureFromFile(sl_pd3dDevice, sl_pd3dDeviceContext, L"assets/test.png", &s_textureResource, &s_shaderResourceView);

	// Setup ImGui binding
	ImGui_ImplDX11_Init(data->hWnd, sl_pd3dDevice, sl_pd3dDeviceContext);

	return true;
}

void graphics::D3D11::Update() {} // TODO

extern void ImGui_ImplDX11_NewFrame();
void graphics::D3D11::ImGuiNewFrame() {
	ImGui_ImplDX11_NewFrame();
}

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool graphics::D3D11::ImGuiHandleEvent(WindowEvent* e) {
	return (ImGui_ImplDX11_WndProcHandler(e->hWnd, e->msg, e->wParam, e->lParam) == 1);
}

void* graphics::D3D11::AddChunk(TempMesh *tempMesh) { 
	assert(sl_pd3dDevice);

	MeshD3D11 mesh = { 0 };

	Vertex* vertices = tempMesh->vertices.data();
	int32_t numVertices = (int32_t) tempMesh->vertices.size();
	int32_t* indices = tempMesh->indices.data();
	int32_t numIndices = (int32_t) tempMesh->indices.size();

#if 1
	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc = {0};
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * numVertices;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData = {0};
	resourceData.pSysMem = vertices;

	HRESULT hr = sl_pd3dDevice->CreateBuffer(&vertexBufferDesc, &resourceData, &mesh.vertexBuffer);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	{
		const char c_szName [] = "Mesh Vertex Buffer";
		mesh.vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
	}

	// Create and initialize the index buffer.
	D3D11_BUFFER_DESC indexBufferDesc = {0};
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.ByteWidth = sizeof(int32_t) * numIndices;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	resourceData.pSysMem = indices;

	hr = sl_pd3dDevice->CreateBuffer(&indexBufferDesc, &resourceData, &mesh.indexBuffer);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	{
		const char c_szName [] = "Mesh Index Buffer";
		mesh.indexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
	}
#else
	// From ImGui
	HRESULT hr;
	static int32_t g_VertexBufferSize, g_IndexBufferSize;
	// Create and grow vertex/index buffers if needed
	if (!mesh.vertexBuffer || g_VertexBufferSize < numVertices)
	{
		if (mesh.vertexBuffer) { mesh.vertexBuffer->Release(); mesh.vertexBuffer = nullptr; }
		g_VertexBufferSize = numVertices + 5000; // TODO: Tweak the extra space
		D3D11_BUFFER_DESC desc = { 0 };
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = g_VertexBufferSize * sizeof(Vertex);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = sl_pd3dDevice->CreateBuffer(&desc, nullptr, &mesh.vertexBuffer);
		g_LogInfo(std::string("Resize vertex buffer: ") + std::to_string(desc.ByteWidth) + std::string(" bytes."));
		if (FAILED(hr))
		{
			__debugbreak();
		}
	}
	if (!mesh.indexBuffer || g_IndexBufferSize < numIndices)
	{
		if (mesh.indexBuffer) { mesh.indexBuffer->Release(); mesh.indexBuffer = nullptr; }
		g_IndexBufferSize = numIndices + 10000;
		D3D11_BUFFER_DESC desc = { 0 };
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = g_IndexBufferSize * sizeof(int32_t);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = sl_pd3dDevice->CreateBuffer(&desc, nullptr, &mesh.indexBuffer);
		g_LogInfo(std::string("Resize index buffer: ") + std::to_string(desc.ByteWidth) + std::string(" bytes."));
		if (FAILED(hr))
		{
			__debugbreak();
		}
	}

	// Copy and convert all vertices into a single contiguous buffer
	D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	hr = sl_pd3dDeviceContext->Map(mesh.vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource);
	if (FAILED(hr))
	{
		__debugbreak();
	}
	hr = sl_pd3dDeviceContext->Map(mesh.indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource);
	if (FAILED(hr))
	{
		__debugbreak();
	}
	Vertex* vtx_dst = (Vertex*)vtx_resource.pData;
	int32_t* idx_dst = (int32_t*)idx_resource.pData;
	memcpy(vtx_dst, vertices, numVertices * sizeof(Vertex));
	memcpy(idx_dst, indices, numIndices * sizeof(int32_t));
	sl_pd3dDeviceContext->Unmap(mesh.vertexBuffer, 0);
	sl_pd3dDeviceContext->Unmap(mesh.indexBuffer, 0);
#endif

	mesh.numIndices = numIndices;

	g_meshes[g_numMeshes] = mesh;

	// Add to draw calls

	DrawCommand cmd = {0};
	cmd.mesh = g_numMeshes;
	//cmd.pipelineState.inputLayout = g_inputlayout
	cmd.pipelineState.numViewports = 1;
	cmd.pipelineState.pixelShader = s_pixelShader;
	cmd.pipelineState.vertexShader = s_vertexShader;
	//cmd.pipelineState.rasterizerState = rasterizerstate
	
	// TODO: Use chunk position to create offset
	// (currently baked in vertex data)
	cmd.worldMatrix = Matrix4::identity();

	g_drawCommands[g_numDrawCommands++] = cmd;

	void* ret = (void*) &g_meshes[g_numMeshes];
	g_numMeshes++;
	return ret;
}

void graphics::D3D11::SetPlayerCameraViewMatrix(Matrix4 viewMatrix) {
	s_view = viewMatrix;
}

void graphics::D3D11::SetProjectionMatrix(Matrix4 projectionMatrix) {
	s_projection = projectionMatrix;
}

// TODO: We're not cleaning up g_meshes or g_numMeshes
// Meaning that we'll go out of bounds
void graphics::D3D11::DeleteChunk(void* data) {
	MeshD3D11* mesh = (MeshD3D11*)data;
	mesh->vertexBuffer->Release();
	mesh->vertexBuffer->Release();
	// TODO: Check if ZERO_MEM is needed
}

#endif // defined(_WIN32) && defined(STARLIGHT_D3D11)
