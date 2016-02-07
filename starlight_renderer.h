#pragma once
#include <cstddef>
#include <Windows.h>

struct ID3D11PixelShader;
struct ID3D11VertexShader;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;

namespace renderer {
	ID3D11PixelShader* CreatePixelShader(const void* ptr, std::size_t size);
	ID3D11VertexShader* CreateVertexShader(const void* ptr, std::size_t size);

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();
	ID3D11RenderTargetView*& GetRenderTargetView();
	ID3D11DepthStencilState* GetDepthStencilState();
	ID3D11DepthStencilView* GetDepthStencilView();


	void SwapBuffers();
	HRESULT Init();
	void Destroy();
	void Clear();
	void Resize(LPARAM lParam);
}
