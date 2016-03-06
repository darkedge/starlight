#include "starlight_windows.h"
#include "starlight_graphics.h"
#include "starlight_game.h"
#include <imgui.h>
#include <atomic>
#include "starlight_thread_safe_queue.h"
#include "starlight_log.h"

// Globals
static std::mutex s_mutex;
static graphics::API* g_renderApi;

static HWND s_hwnd = nullptr;
static util::ThreadSafeQueue<WindowEvent> s_queue;
static std::atomic_bool s_running;

static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;

// This is ugly
#ifdef STARLIGHT_D3D11
static graphics::D3D11 d3d11;
#endif
#ifdef STARLIGHT_D3D10
static graphics::D3D10 d3d10;
#endif

float platform::CalculateDeltaTime() {
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	float deltaTime = float(currentTime.QuadPart - s_lastTime.QuadPart) / float(s_perfFreq.QuadPart);
	s_lastTime = currentTime;
	return deltaTime;
}

// Try to initialize the specified rendering API.
// TODO: Remove globals for the APIs
// If it fails, it returns false.
bool LoadRenderApiImpl(EGraphicsApi e) {
	graphics::API* api = nullptr;
	switch (e) {
#ifdef STARLIGHT_D3D10
	case D3D10:
		logger::LogInfo("Loading D3D10...");
		api = &d3d10;
		break;
#endif
#ifdef STARLIGHT_D3D11
	case D3D11:
		logger::LogInfo("Loading D3D11...");
		api = &d3d11;
		break;
#endif
	default:
		logger::LogInfo("The requested graphics API is not enabled.");
		return false;
	}

	// Cannot init if there is no window
	assert(s_hwnd);

	PlatformData platformData;
	ZeroMemory(&platformData, sizeof(platformData));
	platformData.hWnd = s_hwnd;

#if 1
	// Init new -> destroy old
	if (api->Init(&platformData)) {
		if (g_renderApi) {
			g_renderApi->Destroy();
		}
		g_renderApi = api;

		// Reload fonts
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF("external/imgui-1.47/extra_fonts/DroidSans.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		return true;
	}
#else
	// Destroy old -> init new
	if (g_renderApi) {
		g_renderApi->Destroy();
		g_renderApi = nullptr;
	}
	if (api->Init(&platformData)) {
		g_renderApi = api;
		return true;
	}

#endif
	return false;
}

void ParseMessages() {
	WindowEvent message;
	// Clear queue for now
	// TODO: Process input
	while (s_queue.Dequeue(&message)) {}
}

void MyThreadFunction() {
	EGraphicsApi graphicsApi = EGraphicsApi::D3D11;
	LoadRenderApiImpl(graphicsApi);

	// Init time
	QueryPerformanceFrequency(&s_perfFreq);
	QueryPerformanceCounter(&s_lastTime);

	GameInfo gameInfo;
	ZeroMemory(&gameInfo, sizeof(gameInfo));
	gameInfo.graphicsApi = graphicsApi;

	// Main loop
	while (s_running.load())
	{
		// Check if graphics API change was requested
		if (gameInfo.graphicsApi != graphicsApi) {
			if (LoadRenderApiImpl(gameInfo.graphicsApi)) {
				graphicsApi = gameInfo.graphicsApi;
			} else {
				gameInfo.graphicsApi = graphicsApi;
			}
		}

		ParseMessages();

		g_renderApi->ImGuiNewFrame();

		game::Update(&gameInfo, g_renderApi);

		// Rendering
		if (std::try_lock(s_mutex)) {
			g_renderApi->Render();
			s_mutex.unlock();
		}
	}

	game::Destroy();
	g_renderApi->Destroy();
	ImGui::Shutdown();
	g_renderApi = nullptr;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	WindowEvent params;
	ZeroMemory(&params, sizeof(params));
	params.hWnd = hWnd;
	params.msg = msg;
	params.wParam = wParam;
	params.lParam = lParam;

	s_queue.Enqueue(params);

	if (g_renderApi && g_renderApi->ImGuiHandleEvent(&params))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_renderApi && wParam != SIZE_MINIMIZED) {
			g_renderApi->Resize((int32_t) LOWORD(lParam), (int32_t) HIWORD(lParam));
			//return 0;
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

int __stdcall main() {
	logger::Init();

	auto className = L"StarlightClassName";

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
	RECT windowRect = { 0, 0, 800, 600 }; // TODO: Config file?
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Calculate window dimensions
	auto windowWidth = windowRect.right - windowRect.left;
	auto windowHeight = windowRect.bottom - windowRect.top;

	s_hwnd = CreateWindowExW(
		0L,
		className,
		L"Starlight",
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
	s_running.store(true);
	std::thread thread(MyThreadFunction);

	// Message loop
	MSG msg;
	while (s_running.load() && GetMessageW(&msg, nullptr, 0, 0))
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	thread.join();

	UnregisterClassW(className, GetModuleHandleW(nullptr));
}
