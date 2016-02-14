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

glm::ivec2 platform::GetWindowSize() {
	RECT clientRect;
	GetClientRect(s_hwnd, &clientRect);
	assert(clientRect.right >= clientRect.left);
	assert(clientRect.bottom >= clientRect.top);
	return glm::ivec2(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
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

	WNDCLASSEXW wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEXW);
	wndClass.style = CS_CLASSDC;// CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = GetModuleHandleW(nullptr);
	wndClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wndClass.lpszClassName = className;

	RegisterClassExW(&wndClass);

	// Get desktop rectangle
	RECT desktopRect;
	GetClientRect(GetDesktopWindow(), &desktopRect);

	// Get window rectangle
	RECT windowRect = { 0, 0, 1280, 720 }; // TODO: Config file?
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Calculate window dimensions
	auto windowWidth = windowRect.right - windowRect.left;
	auto windowHeight = windowRect.bottom - windowRect.top;

	s_hwnd = CreateWindowW(
		className,
		L"This is the title bar",
		WS_OVERLAPPEDWINDOW,
		desktopRect.right / 2 - windowWidth / 2,
		desktopRect.bottom / 2 - windowHeight / 2,
		windowWidth,
		windowHeight,
		nullptr,
		nullptr,
		GetModuleHandleW(nullptr),
		nullptr
	);

	// Show the window
	ShowWindow(s_hwnd, SW_SHOWDEFAULT);
	UpdateWindow(s_hwnd);

	if (renderer::Init() < 0)
	{
		// Failed to created D3D device
		renderer::Destroy();
		UnregisterClassW(className, GetModuleHandleW(nullptr));
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
	game::Destroy();
	renderer::Destroy();
	UnregisterClassW(className, GetModuleHandleW(nullptr));

	return 0;
}
#endif
