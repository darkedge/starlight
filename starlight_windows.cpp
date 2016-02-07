#include "starlight_platform.h"
// Windows-specific platform functions.
#ifdef _WIN32

// ImGui implementation

#include "starlight_game.h"
#include "starlight_renderer.h"

#include <imgui.h>
#include "imgui_impl_dx11.h"

static HWND s_hwnd = nullptr;

HWND platform::GetHWND() {
	return s_hwnd;
}

glm::uvec2 platform::GetWindowSize() {
	RECT clientRect;
	GetClientRect(s_hwnd, &clientRect);
	assert(clientRect.right >= clientRect.left);
	assert(clientRect.bottom >= clientRect.top);
	return glm::uvec2(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
}


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

//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
int main()
{
	auto className = L"ImGui Example";
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
		className,
		nullptr
	};
	RegisterClassExW(&wc);

	s_hwnd = CreateWindowW(
		className,
		L"This is the title bar",
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

	// Show the window
	ShowWindow(s_hwnd, SW_SHOWDEFAULT);
	UpdateWindow(s_hwnd);

	if (renderer::Init() < 0)
	{
		// Failed to created D3D device
		renderer::Destroy();
		UnregisterClassW(className, wc.hInstance);
		return 1;
	}

	// Setup ImGui binding (TODO: Write my own version)
	{
		ImGui_ImplDX11_Init(s_hwnd, renderer::GetDevice(), renderer::GetDeviceContext());

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
	UnregisterClassW(className, wc.hInstance);

	return 0;
}
#endif
