#include "starlight_renderer.h"
#if defined(_WIN32) && defined(STARLIGHT_D3D11)
#include "starlight_d3d_shared.h"
#include "starlight_d3d11.h"
#include "starlight_renderer_windows.h"

#include <d3d11.h>
#include <imgui.h>
#include <algorithm>

#include <array>
#include "starlight_log.h"

// Include ImGui implementation
#include "imgui_impl_dx11.h"

// shaders (generated)
// changed to defines to prevent visual studio hanging
#include "SimplePixelShader.h"
#define PixelShaderBlob g_SimplePixelShader
#include "SimpleVertexShader.h"
#define VertexShaderBlob g_SimpleVertexShader

static ID3D11RasterizerState* s_rasterizerState;
static ID3D11PixelShader* s_pixelShader;
static ID3D11VertexShader* s_vertexShader;
static ID3D11InputLayout* s_inputLayout;

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

void renderer::D3D11::Render() {
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


void renderer::D3D11::Destroy()
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

void renderer::D3D11::Resize(int32_t width, int32_t height) {
	SafeRelease(s_renderTargetView);
	D3D_TRY(s_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
	CreateRenderTarget();
}

// TODO: Any error here is unrecoverable and should return false
bool renderer::D3D11::Init(PlatformData *data)
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

int32_t renderer::D3D11::UploadMesh(Vertex* vertices, int32_t numVertices, int32_t* indices, int32_t numIndices) {
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

void renderer::D3D11::Update() {
	// TODO
}

void renderer::D3D11::ImGuiNewFrame() {
	ImGui_ImplDX11_NewFrame();
}

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool renderer::D3D11::ImGuiHandleEvent(WindowEvent* e) {
	if (ImGui_ImplDX11_WndProcHandler(e->hWnd, e->msg, e->wParam, e->lParam)) {
		return true;
	}
	return false;
}

void renderer::D3D11::SetPlayerCameraViewMatrix(glm::mat4 matrix) {
	// TODO
}

void renderer::D3D11::SetProjectionMatrix(glm::mat4 matrix) {
	// TODO
}

#endif // defined(_WIN32) && defined(STARLIGHT_D3D11)
