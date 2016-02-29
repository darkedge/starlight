#include "starlight_windows.h"
#include "starlight_renderer.h"
#include "starlight_game.h"
#include <imgui.h>
#include <atomic>
#include "starlight_thread_safe_queue.h"
#include "starlight_log.h"

// Globals
static std::mutex s_mutex;
static renderer::IGraphicsApi* g_renderApi;

static HWND s_hwnd = nullptr;
static util::ThreadSafeQueue<WindowEvent> s_queue;
static std::atomic_bool s_running;

static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;

static renderer::D3D11 d3d11;
static renderer::D3D10 d3d10;

static bool switchApi;
static EGraphicsApi nextApi;

float platform::CalculateDeltaTime() {
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	float deltaTime = float(currentTime.QuadPart - s_lastTime.QuadPart) / float(s_perfFreq.QuadPart);
	s_lastTime = currentTime;
	return deltaTime;
}

// Try to initialize the specified rendering API.
// If it fails, it returns false.
bool LoadRenderApiImpl(EGraphicsApi e) {
	renderer::IGraphicsApi* api = nullptr;
	switch (e) {
	case D3D11:
		api = &d3d11;
		break;
	case D3D10:
		api = &d3d10;
		break;
	default:
		logger::LogInfo("The requested graphics API is not implemented on this platform.");
		break;
	}

	// Requested API is same as current API
	//if (g_renderApi == api) return true;

	// Cannot init if there is no window
	assert(s_hwnd);

	PlatformData platformData;
	ZeroMemory(&platformData, sizeof(platformData));
	platformData.hWnd = s_hwnd;

#if 1
	if (g_renderApi == api) {
		g_renderApi->Destroy();
		g_renderApi->Init(&platformData);
		return true;
	}
	// This goes bad if the same api is requested twice
	if (api->Init(&platformData)) {
		if (g_renderApi) {
			g_renderApi->Destroy();
		}
		g_renderApi = api;
		return true;
	}
#else
	// Destroy first, init after
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

bool platform::LoadRenderApi(EGraphicsApi e) {
	switchApi = true;
	nextApi = e;

	// TODO: Change signature to void
	return true;
}

void ParseMessages() {
	WindowEvent message;
	// Clear queue for now
	// TODO: Process input
	while (s_queue.Dequeue(&message)) {}
}

void MyThreadFunction() {
	// Load D3D11
	bool success = false;

#ifdef STARLIGHT_D3D11
	if (!success) {
		logger::LogInfo("Loading Direct3D 11...");
		success = LoadRenderApiImpl(D3D11);
	}
#endif
#ifdef STARLIGHT_D3D10
	if (!success) {
		logger::LogInfo("Loading Direct3D 10...");
		success = LoadRenderApiImpl(D3D10);
	}
#endif
	if (!success) {
		MessageBoxW(nullptr, L"No renderer available!", nullptr, MB_OK);
		s_running.store(false);
	}

	game::Init(g_renderApi);

	// Init time
	QueryPerformanceFrequency(&s_perfFreq);
	QueryPerformanceCounter(&s_lastTime);

	// Main loop
	while (s_running.load())
	{
		if (switchApi) {
			switchApi = false;
			LoadRenderApiImpl(nextApi);
		}
		ParseMessages();

		g_renderApi->ImGuiNewFrame();

		game::Update(g_renderApi);

		// Rendering
		if (std::try_lock(s_mutex)) {
			g_renderApi->Render();
			s_mutex.unlock();
		}
	}

	game::Destroy();
	g_renderApi->Destroy();
	g_renderApi = nullptr;
}

#if 0
// Perhaps better suited for the renderer
glm::ivec2 platform::GetWindowSize() {
	RECT clientRect;
	GetClientRect(s_hwnd, &clientRect);
	assert(clientRect.right >= clientRect.left);
	assert(clientRect.bottom >= clientRect.top);
	return glm::ivec2(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
}
#endif

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

int __cdecl main()
{
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
	RECT windowRect = { 0, 0, 1280, 720 }; // TODO: Config file?
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
