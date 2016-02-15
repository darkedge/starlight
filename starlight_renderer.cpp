#include "starlight_renderer.h"
#include "starlight_platform.h"
#include <d3d11.h>
#include <imgui.h> // for the clear color

#include "starlight_log.h"
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

// Direct3D 11 device
static ID3D11Device* s_device = nullptr;
static ID3D11DeviceContext* s_deviceContext = nullptr;
static IDXGISwapChain* s_swapChain = nullptr;
static ID3D11RenderTargetView* s_mainRenderTargetView = nullptr;
static ID3D11DepthStencilState* s_depthStencilState = nullptr;
static ID3D11DepthStencilView* s_depthStencilView = nullptr;
static ID3D11Texture2D* s_depthStencilBuffer = nullptr;

void CleanupRenderTarget();
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
	return s_mainRenderTargetView;
}

ID3D11DepthStencilState* renderer::GetDepthStencilState()
{
	return s_depthStencilState;
}

ID3D11DepthStencilView* renderer::GetDepthStencilView()
{
	return s_depthStencilView;
}

void renderer::SwapBuffers()
{
	s_swapChain->Present(0, 0);
}

void renderer::Destroy()
{
	SafeRelease(s_device);
	SafeRelease(s_deviceContext);
	SafeRelease(s_swapChain);
	SafeRelease(s_mainRenderTargetView);
	SafeRelease(s_depthStencilState);
	SafeRelease(s_depthStencilView);
	SafeRelease(s_depthStencilBuffer);
}

void renderer::Clear()
{
	ImVec4 clear_col = ImColor(114, 144, 154);
	s_deviceContext->ClearRenderTargetView(s_mainRenderTargetView, (float*)&clear_col);
	s_deviceContext->ClearDepthStencilView(s_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void renderer::Resize(LPARAM lParam)
{
	CleanupRenderTarget();
	D3D_TRY(s_swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0));
	CreateRenderTarget();
}

void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	s_swapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	D3D_TRY(s_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer));
	D3D_TRY(s_device->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &s_mainRenderTargetView));
	s_deviceContext->OMSetRenderTargets(1, &s_mainRenderTargetView, nullptr);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (s_mainRenderTargetView) {
		s_mainRenderTargetView->Release();
		s_mainRenderTargetView = nullptr;
	}
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

	//const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
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
	ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;
	D3D_TRY(renderer::GetDevice()->CreateDepthStencilState(&depthStencilStateDesc, &s_depthStencilState));

	return S_OK;
}
