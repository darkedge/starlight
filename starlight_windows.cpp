// Windows-specific platform functions.
#ifdef _WIN32

#include "imgui.cpp"

// These are temporary, ImGui draw layer needs to be rewritten
// and the demo file will be removed once I actually use ImGui
#include "imgui_draw.cpp"

#include <imgui.h>
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "starlight_game.h"
#include "starlight_renderer.h"

//#define NOWIDE_MSVC
#include <nowide/convert.hpp>

// Data

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (renderer::GetDevice() != nullptr && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX11_InvalidateDeviceObjects();
			renderer::Resize(lParam);
			ImGui_ImplDX11_CreateDeviceObjects();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Create application window
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0L,
		0L,
		GetModuleHandleW(nullptr),
		nullptr,
		LoadCursorW(nullptr, IDC_ARROW),
		nullptr,
		nullptr,
		L"ImGui Example",
		nullptr
	};
	RegisterClassExW(&wc);

	HWND hwnd = CreateWindowW(
		L"ImGui Example",
		L"ImGui DirectX11 Example",
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		1280,
		800,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	if (renderer::Init(hwnd) < 0)
	{
		// Failed to created D3D device
		renderer::Destroy();
		UnregisterClassW(L"ImGui Example", wc.hInstance);
		return 1;
	}

	// Show the window
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	// Setup ImGui binding (TODO: Write my own version)
	{
		ImGui_ImplDX11_Init(hwnd, renderer::GetDevice(), renderer::GetDeviceContext());

		// Load Fonts
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("external/imgui-1.47/extra_fonts/DroidSans.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	}

	game::Init();

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			continue;
		}
		ImGui_ImplDX11_NewFrame();

		// Rendering
		renderer::Clear();

		game::Update();

		ImGui::Render();
		renderer::SwapBuffers();
	}

	ImGui_ImplDX11_Shutdown();
	renderer::Destroy();
	UnregisterClassW(L"ImGui Example", wc.hInstance);

	return 0;
}
#endif
