#define STARLIGHT_D3D11
#include "starlight_graphics.h"
#if defined(_WIN32) && defined(STARLIGHT_D3D11)

#include "starlight_d3d11.h"
#include "starlight_renderer_windows.h"

#include "starlight_log.h"
#include "starlight_game.h"

#include "starlight_hlsl_generated.h"
#include "stb_image.h"

#include <codecvt> // wstring to string

using namespace Vectormath::Aos;

struct MeshD3D11 {
	union {
		struct {
			ID3D11Buffer* vertexBuffer;
			ID3D11Buffer* indexBuffer;
			int32_t numIndices;
			int2 xz;
		} live;
		MeshD3D11* next;
	} state;
};

// These are free lists
// Might not be efficient because we iterate over every command every frame
#define MAX_DRAW_COMMANDS 1024
static DrawCommand g_drawCommands[MAX_DRAW_COMMANDS];
static DrawCommand* firstAvailableDrawCommand;

#define MAX_MESHES 1024
static MeshD3D11 g_meshes[MAX_MESHES];
static MeshD3D11* firstAvailableMesh;

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
static ID3D11BlendState *s_blendState;

static Matrix4 s_view;
static Matrix4 s_projection;

// Should be atomic read/write
static bool g_resize = false;

#include "examples\directx11_example\imgui_impl_dx11.h"

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
	if (s_depthStencilBuffer) { s_depthStencilBuffer->Release(); s_depthStencilBuffer = NULL; }
	if (s_depthStencilView) { s_depthStencilView->Release(); s_depthStencilView = NULL; }
	if (s_depthStencilState) { s_depthStencilState->Release(); s_depthStencilState = NULL; }
	if (s_blendState) { s_blendState->Release(); s_blendState = NULL; }
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

	// Log graphics adapter
	IDXGIDevice* pDXGIDevice;
	D3D_TRY(sl_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice));
	IDXGIAdapter* pDXGIAdapter;
	pDXGIDevice->GetAdapter(&pDXGIAdapter);
	DXGI_ADAPTER_DESC adapterDesc;
	pDXGIAdapter->GetDesc(&adapterDesc);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string converted_str = converter.to_bytes( std::wstring(adapterDesc.Description) );
	g_LogInfo(converted_str);

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

// TODO: This probably misses some stuff
void graphics::D3D11::Destroy() {
	ImGui_ImplDX11_Shutdown();
	CleanupDeviceD3D();
	if (s_shaderResourceView) { s_shaderResourceView->Release(); s_shaderResourceView = NULL; }
}

struct bound3 {
	float mMinX;
	float mMinY;
	float mMinZ;
	float mMaxX;
	float mMaxY;
	float mMaxZ;
};

struct frustum3 {
	Vector4 mPlane[6];
	//Vector3 mPoints[8]; // corner points
};

// http://www.iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
bool boxInFrustum( frustum3* fru, bound3* box )
{
    // check box outside/inside of frustum
    for ( size_t i = 0; i < 6; i++ )
    {
        int out = 0;
        out += ((dot( fru->mPlane[i], Vector4(box->mMinX, box->mMinY, box->mMinZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMaxX, box->mMinY, box->mMinZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMinX, box->mMaxY, box->mMinZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMaxX, box->mMaxY, box->mMinZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMinX, box->mMinY, box->mMaxZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMaxX, box->mMinY, box->mMaxZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMinX, box->mMaxY, box->mMaxZ, 1.0f) ) < 0.0f ) ? 1: 0);
        out += ((dot( fru->mPlane[i], Vector4(box->mMaxX, box->mMaxY, box->mMaxZ, 1.0f) ) < 0.0f ) ? 1: 0);
        if( out == 8 ) return false;
    }

    #if 0 // Additional checks

    // check frustum outside/inside box
    int out;
    out = 0; for( int i = 0; i < 8; i++ ) out += ((fru.mPoints[i].x > box.mMaxX) ? 1: 0); if ( out == 8 ) return false;
    out = 0; for( int i = 0; i < 8; i++ ) out += ((fru.mPoints[i].x < box.mMinX) ? 1: 0); if ( out == 8 ) return false;
    out = 0; for( int i = 0; i < 8; i++ ) out += ((fru.mPoints[i].y > box.mMaxY) ? 1: 0); if ( out == 8 ) return false;
    out = 0; for( int i = 0; i < 8; i++ ) out += ((fru.mPoints[i].y < box.mMinY) ? 1: 0); if ( out == 8 ) return false;
    out = 0; for( int i = 0; i < 8; i++ ) out += ((fru.mPoints[i].z > box.mMaxZ) ? 1: 0); if ( out == 8 ) return false;
    out = 0; for( int i = 0; i < 8; i++ ) out += ((fru.mPoints[i].z < box.mMinZ) ? 1: 0); if ( out == 8 ) return false;
    #endif

    return true;
}

void FrustumFromVPMatrix(frustum3* frustum, Matrix4* vp) {
	// left
	frustum->mPlane[0] =  Vector4(
		vp->getElem(0, 3) + vp->getElem(0, 0),
		vp->getElem(1, 3) + vp->getElem(1, 0),
		vp->getElem(2, 3) + vp->getElem(2, 0),
		vp->getElem(3, 3) + vp->getElem(3, 0)
	);
	// right
	frustum->mPlane[1] = Vector4(
		vp->getElem(0, 3) - vp->getElem(0, 0),
		vp->getElem(1, 3) - vp->getElem(1, 0),
		vp->getElem(2, 3) - vp->getElem(2, 0),
		vp->getElem(3, 3) - vp->getElem(3, 0)
	);
	// bottom
	frustum->mPlane[2] = Vector4(
		vp->getElem(0, 3) + vp->getElem(0, 1),
		vp->getElem(1, 3) + vp->getElem(1, 1),
		vp->getElem(2, 3) + vp->getElem(2, 1),
		vp->getElem(3, 3) + vp->getElem(3, 1)
	);
	// top
	frustum->mPlane[3] = Vector4(
		vp->getElem(0, 3) - vp->getElem(0, 1),
		vp->getElem(1, 3) - vp->getElem(1, 1),
		vp->getElem(2, 3) - vp->getElem(2, 1),
		vp->getElem(3, 3) - vp->getElem(3, 1)
	);
	// near
	frustum->mPlane[4] = Vector4(
		vp->getElem(0, 2),
		vp->getElem(1, 2),
		vp->getElem(2, 2),
		vp->getElem(3, 2)
	);
	// far
	frustum->mPlane[5] = Vector4(
		vp->getElem(0, 3) - vp->getElem(0, 2),
		vp->getElem(1, 3) - vp->getElem(1, 2),
		vp->getElem(2, 3) - vp->getElem(2, 2),
		vp->getElem(3, 3) - vp->getElem(3, 2)
	);
}

void graphics::D3D11::Render() {
	if (g_resize) {
		CleanupRenderTarget();
		D3D_TRY(g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
		CreateRenderTarget();
		g_resize = false;
	}

	// Clear
	ImVec4 clear_col = ImColor(114, 144, 154);
	sl_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*) &clear_col);
	sl_pd3dDeviceContext->ClearDepthStencilView(s_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Bubble sort draw commands
	// Currently just contains a single chunk
	// No need to clear the buffer
	/*
	bool sorting = true;
	while (sorting) {
		sorting = false;
		for (size_t i = 1; i < g_lastDrawCommand; i++) {
			if (g_drawCommands[i].state.live.key < g_drawCommands[i - 1].state.live.key ) {
				DrawCommand t = g_drawCommands[i];
				g_drawCommands[i] = g_drawCommands[i - 1];
				g_drawCommands[i - 1] = t;
				sorting = true;
			}
		}
	}
	*/

	// For retrieving the window size
	DXGI_SWAP_CHAIN_DESC desc;
	g_pSwapChain->GetDesc(&desc);

	// Constant Buffers
	// TODO: z-min, z-max (optional), FOV!
	if (desc.BufferDesc.Width > 0 && desc.BufferDesc.Height > 0) {
		s_projection = Matrix4::perspective(45.0f * DEG2RAD, (float) desc.BufferDesc.Width / (float) desc.BufferDesc.Height, 0.1f, 1000.0f);
		assert(s_constantBuffers[EConstantBuffer::Projection]);
		sl_pd3dDeviceContext->UpdateSubresource(s_constantBuffers[EConstantBuffer::Projection], 0, nullptr, &s_projection, 0, 0);
	}

	assert(s_constantBuffers[EConstantBuffer::View]);
	sl_pd3dDeviceContext->UpdateSubresource(s_constantBuffers[EConstantBuffer::View], 0, nullptr, &s_view, 0, 0);

	size_t numTrianglesDrawn = 0;

	Matrix4 vp = s_projection * s_view;
	frustum3 frustum;
	FrustumFromVPMatrix(&frustum, &vp);

	int32_t culledChunks = 0;

	// Submit draw commands
	for (size_t i = 0; i < MAX_DRAW_COMMANDS; i++) {
		DrawCommand* cmd = &g_drawCommands[i];

		MeshD3D11* mesh = cmd->state.live.mesh;
		if (!mesh) continue;

		// Frustum culling
		bound3 b = {
			(float) mesh->state.live.xz.x,
			0.0f,
			(float) mesh->state.live.xz.z,
			(float) mesh->state.live.xz.x + CHUNK_DIM_XZ + 1.0f,
			(float) CHUNK_DIM_Y + 1.0f,
			(float) mesh->state.live.xz.z + CHUNK_DIM_XZ + 1.0f
		};
		if (!boxInFrustum(&frustum, &b)) {
			culledChunks++;
			continue;
		};

		PipelineState* pipelineState = &cmd->state.live.pipelineState;

		// Constant Buffers
		assert(s_constantBuffers[EConstantBuffer::Model]);
		sl_pd3dDeviceContext->UpdateSubresource(s_constantBuffers[EConstantBuffer::Model], 0, nullptr, &cmd->state.live.worldMatrix, 0, 0);

		// Input Assembler
		uint32_t stride = sizeof(Vertex);
		uint32_t offset = 0;
		//assert(pipelineState.inputLayout);
		//sl_pd3dDeviceContext->IASetInputLayout(pipelineState.inputLayout);
		sl_pd3dDeviceContext->IASetInputLayout(s_inputLayout);
		sl_pd3dDeviceContext->IASetVertexBuffers(0, 1, &mesh->state.live.vertexBuffer, &stride, &offset);
		sl_pd3dDeviceContext->IASetIndexBuffer(mesh->state.live.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
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
		sl_pd3dDeviceContext->OMSetBlendState(s_blendState, NULL, 0xFFFFFFFF);
		sl_pd3dDeviceContext->OMSetDepthStencilState(s_depthStencilState, 1);

		// Draw call
		sl_pd3dDeviceContext->DrawIndexed(mesh->state.live.numIndices, 0, 0);

		numTrianglesDrawn += mesh->state.live.numIndices / 3;
	}

	ImGui::Begin("starlight_d3d11");
	ImGui::Text("Triangles drawn: %zi", numTrianglesDrawn);
	static bool wireframe = false;
	if (ImGui::Checkbox("Wireframe", &wireframe)) {
		D3D11_RASTERIZER_DESC rasterizerDesc;
		s_rasterizerState->GetDesc( &rasterizerDesc );
		s_rasterizerState->Release();
		rasterizerDesc.FillMode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
		D3D_TRY(sl_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &s_rasterizerState));
		sl_pd3dDeviceContext->RSSetState( s_rasterizerState );
	}
	ImGui::Text("Culled chunks: %i / %i", culledChunks, CHUNK_DIAMETER * CHUNK_DIAMETER);
	ImGui::End();

	ImGui::Render();

	// Present
	g_pSwapChain->Present(1, 0);
}

void graphics::D3D11::Resize(int32_t, int32_t) {
	g_resize = true;
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

	// Create texture
	{
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	    D3D11_TEXTURE2D_DESC desc;
	    desc.Width = 16;
	    desc.Height = 16;
	    desc.MipLevels = 0;
	    desc.ArraySize = 256;
	    desc.Format = format;
	    desc.SampleDesc.Count = 1;
	    desc.SampleDesc.Quality = 0;
	    desc.Usage = D3D11_USAGE_DEFAULT;
	    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	    desc.CPUAccessFlags = 0;
	    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	    ID3D11Texture2D* texArray = nullptr;
	    HRESULT hr = sl_pd3dDevice->CreateTexture2D( &desc, nullptr, &texArray );
	    assert(SUCCEEDED(hr) && texArray != 0);

	    texArray->GetDesc(&desc);

	    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
	    SRVDesc.Format = format;
	    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	    SRVDesc.Texture2DArray.MostDetailedMip = 0;
	    SRVDesc.Texture2DArray.MipLevels = (UINT)-1;
	    SRVDesc.Texture2DArray.FirstArraySlice = 0;
	    SRVDesc.Texture2DArray.ArraySize = 256;

	    hr = sl_pd3dDevice->CreateShaderResourceView( texArray, &SRVDesc, &s_shaderResourceView );
	    assert(SUCCEEDED(hr));

		int width, height, n;
		unsigned char* img = stbi_load("../assets/terrain.png", &width, &height, &n, 4);
		assert(img);

		// Split image into texture array
		for (UINT i = 0; i < 256; i++) {
			UINT x = i % 16;
			UINT y = i / 16;
			unsigned char* src = img + (x * 16 + y * 16 * 256) * n;
			sl_pd3dDeviceContext->UpdateSubresource(texArray, D3D11CalcSubresource(0, i, desc.MipLevels), 0, src, width * n, 0);			
		}

	    sl_pd3dDeviceContext->GenerateMips( s_shaderResourceView );

	    s_textureResource = texArray;

		texArray->Release();
		stbi_image_free(img);
	}

	// Setup ImGui binding
	ImGui_ImplDX11_Init(data->hWnd, sl_pd3dDevice, sl_pd3dDeviceContext);

	// Init free lists
	firstAvailableDrawCommand = &g_drawCommands[0];
	for (size_t i = 0; i < MAX_DRAW_COMMANDS; i++) {
		g_drawCommands[i].state.next = &g_drawCommands[i + 1];
	}
	g_drawCommands[MAX_DRAW_COMMANDS - 1].state.next = nullptr;

	firstAvailableMesh = &g_meshes[0];
	for (size_t i = 0; i < MAX_MESHES; i++) {
		g_meshes[i].state.next = &g_meshes[i + 1];
	}
	g_meshes[MAX_MESHES - 1].state.next = nullptr;

	{
		D3D11_BLEND_DESC bs = {};
		for (int i = 0; i < 8; i++)	{
			bs.RenderTarget[i].BlendEnable = TRUE;
			bs.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
			bs.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_MAX;
			bs.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bs.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bs.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bs.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
			bs.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		}

		D3D_TRY(sl_pd3dDevice->CreateBlendState(&bs, &s_blendState));
	}

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

	assert(firstAvailableMesh);
	MeshD3D11* mesh = firstAvailableMesh;
	firstAvailableMesh = firstAvailableMesh->state.next;
	assert(firstAvailableMesh);
	ZERO_MEM(mesh, sizeof(MeshD3D11));

	mesh->state.live.xz = tempMesh->xz;

	Vertex* vertices = tempMesh->vertices.data();
	int32_t numVertices = (int32_t) tempMesh->vertices.size();
	assert(numVertices);
	int32_t* indices = tempMesh->indices.data();
	int32_t numIndices = (int32_t) tempMesh->indices.size();
	assert(numIndices);

#if 1
	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc = {0};
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * numVertices;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData = {0};
	resourceData.pSysMem = vertices;

	HRESULT hr = sl_pd3dDevice->CreateBuffer(&vertexBufferDesc, &resourceData, &mesh->state.live.vertexBuffer);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	{
		const char c_szName [] = "Mesh Vertex Buffer";
		mesh->state.live.vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
	}

	// Create and initialize the index buffer.
	D3D11_BUFFER_DESC indexBufferDesc = {0};
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.ByteWidth = sizeof(int32_t) * numIndices;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	resourceData.pSysMem = indices;

	hr = sl_pd3dDevice->CreateBuffer(&indexBufferDesc, &resourceData, &mesh->state.live.indexBuffer);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	{
		const char c_szName [] = "Mesh Index Buffer";
		mesh->state.live.indexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
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

	mesh->state.live.numIndices = numIndices;

	//char buf[64];
	//sprintf(buf, "Create chunk index %zi", mesh - g_meshes);
	//g_LogInfo(buf);

	// Add to draw calls

	assert(firstAvailableDrawCommand);
	DrawCommand* cmd = firstAvailableDrawCommand;
	firstAvailableDrawCommand = firstAvailableDrawCommand->state.next;
	assert(firstAvailableDrawCommand);
	ZERO_MEM(cmd, sizeof(DrawCommand));
	//cmd->state.live.mesh = g_numMeshes;
	cmd->state.live.mesh = mesh;
	//cmd.pipelineState.inputLayout = g_inputlayout
	cmd->state.live.pipelineState.numViewports = 1;
	cmd->state.live.pipelineState.pixelShader = s_pixelShader;
	cmd->state.live.pipelineState.vertexShader = s_vertexShader;
	//cmd.pipelineState.rasterizerState = rasterizerstate
	// TODO: Use chunk position to create offset
	// (currently baked in vertex data)
	Matrix4 a = Matrix4::identity();
	memcpy(&cmd->state.live.worldMatrix, &a, sizeof(Matrix4));

	return mesh;
}

void graphics::D3D11::SetPlayerCameraViewMatrix(Matrix4 viewMatrix) {
	s_view = viewMatrix;
}

void graphics::D3D11::SetProjectionMatrix(Matrix4 projectionMatrix) {
	s_projection = projectionMatrix;
}

void graphics::D3D11::DeleteChunk(void* data) {
	MeshD3D11* mesh = (MeshD3D11*)data;
	mesh->state.live.vertexBuffer->Release();
	mesh->state.live.indexBuffer->Release();
	ZERO_MEM(mesh, sizeof(MeshD3D11));

	mesh->state.next = firstAvailableMesh;
	firstAvailableMesh = mesh;

	// Also delete corresponding draw command
	size_t ndx = mesh - g_meshes;
	DrawCommand* cmd = &g_drawCommands[ndx];
	cmd->state.live.mesh = NULL;
	cmd->state.next = firstAvailableDrawCommand;
	firstAvailableDrawCommand = cmd;

	//char buf[64];
	//sprintf(buf, "Delete chunk index %zi", ndx);
	//g_LogInfo(buf);
}

size_t graphics::D3D11::GetDataIndex(void* ptr) {
	return ((MeshD3D11*)ptr) - &g_meshes[0];
}

#endif // defined(_WIN32) && defined(STARLIGHT_D3D11)
