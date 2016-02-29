#include "starlight_renderer.h"
#if defined(_WIN32) && defined(STARLIGHT_D3D10)
#include "starlight_d3d_shared.h"
#include "starlight_d3d10.h"
#include "starlight_renderer_windows.h"

#include <imgui.h>

#include "imgui_impl_dx10.h"


#include <d3d10_1.h>
#include <d3d10.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

// Data
static ID3D10Device*            g_pd3dDevice = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D10RenderTargetView*  g_mainRenderTargetView = nullptr;

void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	g_pSwapChain->GetDesc(&sd);

	// Create the render target
	ID3D10Texture2D* pBackBuffer;
	D3D10_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
	g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
	g_pd3dDevice->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

HRESULT CreateDeviceD3D(HWND hWnd)
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
	//createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	if (D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice) != S_OK)
		return E_FAIL;

	// Setup rasterizer
	{
		D3D10_RASTERIZER_DESC RSDesc;
		memset(&RSDesc, 0, sizeof(D3D10_RASTERIZER_DESC));
		RSDesc.FillMode = D3D10_FILL_SOLID;
		RSDesc.CullMode = D3D10_CULL_NONE;
		RSDesc.FrontCounterClockwise = FALSE;
		RSDesc.DepthBias = 0;
		RSDesc.SlopeScaledDepthBias = 0.0f;
		RSDesc.DepthBiasClamp = 0;
		RSDesc.DepthClipEnable = TRUE;
		RSDesc.ScissorEnable = TRUE;
		RSDesc.AntialiasedLineEnable = FALSE;
		RSDesc.MultisampleEnable = (sd.SampleDesc.Count > 1) ? TRUE : FALSE;

		ID3D10RasterizerState* pRState = nullptr;
		g_pd3dDevice->CreateRasterizerState(&RSDesc, &pRState);
		g_pd3dDevice->RSSetState(pRState);
		pRState->Release();
	}

	CreateRenderTarget();

	return S_OK;
}

extern LRESULT ImGui_ImplDX10_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool renderer::D3D10::ImGuiHandleEvent(WindowEvent* e) {
	return (ImGui_ImplDX10_WndProcHandler(e->hWnd, e->msg, e->wParam, e->lParam) == 1);
}

void renderer::D3D10::Resize(int32_t width, int32_t height) {
	CleanupRenderTarget();
	g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTarget();
}

extern void ImGui_ImplDX10_NewFrame();
void renderer::D3D10::ImGuiNewFrame() {
	ImGui_ImplDX10_NewFrame();
}

void CleanupDeviceD3D() {
	CleanupRenderTarget();
	SafeRelease(g_pSwapChain);
	SafeRelease(g_pd3dDevice);
}

bool renderer::D3D10::Init(PlatformData* data)
{
	if (CreateDeviceD3D(data->hWnd) != S_OK) {
		CleanupDeviceD3D();
		return false;
	}

	ImGui_ImplDX10_Init(data->hWnd, g_pd3dDevice);

	{
		// Load Fonts
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("external/imgui-1.47/extra_fonts/DroidSans.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	}

	return true;
}

void renderer::D3D10::Render() {
	// Clear
	ImVec4 clear_col = ImColor(114, 144, 154);
	g_pd3dDevice->ClearRenderTargetView(g_mainRenderTargetView, (float*) &clear_col);

	// TODO: Game rendering here

	ImGui::Render();

	// Present
	g_pSwapChain->Present(0, 0);
}

void renderer::D3D10::Destroy() {
	ImGui_ImplDX10_Shutdown();
	CleanupDeviceD3D();
}

void renderer::D3D10::Update() {
	// TODO
}

void renderer::D3D10::SetPlayerCameraViewMatrix(glm::mat4 matrix) {
	// TODO
}

void renderer::D3D10::SetProjectionMatrix(glm::mat4 matrix) {
	// TODO
}

int32_t renderer::D3D10::UploadMesh(Vertex* vertices, int32_t numVertices, int32_t* indices, int32_t numIndices) {
	// TOOD
	return -1;
}

#endif
