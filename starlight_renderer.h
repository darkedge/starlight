#pragma once
#include <cstddef>
#include <Windows.h>

struct ID3D11PixelShader;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;

namespace renderer {
	void Init();

	ID3D11PixelShader* CreatePixelShader(const void* ptr, std::size_t size);
	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();
	void SwapBuffers();
	HRESULT Init(HWND hwnd);
	void Destroy();
	void Clear();
	void Resize(LPARAM lParam);
}