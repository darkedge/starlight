#include "starlight_platform.h"
// Windows-specific platform functions.
#ifdef _WIN32

// ImGui implementation

#include "starlight_game.h"
#include "starlight_renderer.h"

#include <imgui.h>
#include "imgui_impl_dx11.h"

#include <atomic>

#include "starlight_thread_safe_queue.h"

static std::mutex s_mutex;

struct WindowMessage {
	HWND hWnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
};

static HWND s_hwnd = nullptr;
static util::ThreadSafeQueue<WindowMessage> s_queue;


DWORD WINAPI MyThreadFunction(LPVOID lpParam);

static std::atomic_bool s_running;

void ParseMessages() {
	WindowMessage message;
	while (s_queue.Dequeue(&message)) {
#if 0
		//HWND hWnd = message.hWnd;
		UINT msg = message.msg;
		WPARAM wParam = message.wParam;
		//LPARAM lParam = message.lParam;

		switch (msg) {
		case WM_SIZE:
			if (renderer::GetDevice() != nullptr && wParam != SIZE_MINIMIZED) {
				renderer::Resize();
				ImGui_ImplDX11_InvalidateDeviceObjects();
				ImGui_ImplDX11_CreateDeviceObjects();
			}
			break;
		default:
			break;
		}
#endif
	}
}

DWORD WINAPI MyThreadFunction(LPVOID lpPAram) {
	if (renderer::Init() < 0)
	{
		// Failed to created D3D device
		renderer::Destroy();
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
	while (s_running.load())
	{
		ParseMessages();

		ImGui_ImplDX11_NewFrame();

		game::Update();

		// Rendering
		if (std::try_lock(s_mutex)) {
			renderer::Clear();
			game::Render();
			ImGui::Render();
			renderer::SwapBuffers();
			s_mutex.unlock();
		}
	}

	ImGui_ImplDX11_Shutdown();
	game::Destroy();
	renderer::Destroy();

	return 0;
}


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


extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	s_queue.Enqueue(WindowMessage{hWnd, msg, wParam, lParam});

	if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (renderer::GetDevice() != nullptr && wParam != SIZE_MINIMIZED) {
			renderer::Resize();
			//ImGui_ImplDX11_InvalidateDeviceObjects();
			//ImGui_ImplDX11_CreateDeviceObjects();
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		s_running.store(false);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main()
{
	auto className = L"ImGui Example";

	WNDCLASSEXW wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEXW);
	wndClass.style = CS_CLASSDC;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = GetModuleHandleW(nullptr);
	wndClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wndClass.lpszClassName = className;

	RegisterClassExW(&wndClass);

	// Get desktop rectangle
	RECT desktopRect;
	GetClientRect(GetDesktopWindow(), &desktopRect);

	// Get window rectangle
	RECT windowRect = { 0, 0, 1600, 900 }; // TODO: Config file?
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Calculate window dimensions
	auto windowWidth = windowRect.right - windowRect.left;
	auto windowHeight = windowRect.bottom - windowRect.top;

	s_running.store(true);

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

	
	// Create thread
	auto thread = CreateThread(
		nullptr,                // default security attributes
		0,                      // use default stack size  
		MyThreadFunction,       // thread function name
		nullptr,          // argument to thread function 
		0,                      // use default creation flags 
		nullptr);   // returns the thread identifier

	// Message loop
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	WaitForSingleObject(thread, INFINITE);

	UnregisterClassW(className, GetModuleHandleW(nullptr));
}
#endif
