//#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG

#include <enet/enet.h>

#define SL_IMPL

#include "starlight_win32.h"
#include "starlight_graphics.h"
#include "starlight_game.h"
#include <imgui.h>
#include <atomic>
#include "starlight_thread_safe_queue.h"
#include "starlight_log.h"
#include "starlight_memory.h"
#include <process.h>

#include "Objbase.h"

#ifdef SL_CL
  static const wchar_t* s_dllName = L"starlight.dll";
#else
  #ifdef SL_UB
    #ifdef _DEBUG
      #ifdef _WIN64
        static const wchar_t* s_dllName = L"starlight_ub_x64_UB Debug.dll";
      #else
        static const wchar_t* s_dllName = L"starlight_ub_Win32_UB Debug.dll";
      #endif
    #endif
  #else
    #ifdef _DEBUG
      #ifdef _WIN64
        static const wchar_t* s_dllName = L"starlight_x64_Debug.dll";
      #else
        static const wchar_t* s_dllName = L"starlight_Win32_Debug.dll";
      #endif
    #endif
  #endif
#endif

// Globals
static std::mutex s_mutex;
static graphics::API* g_renderApi;

static HWND s_hwnd = nullptr;
static util::ThreadSafeQueue<WindowEvent>* s_queue;
static std::atomic_bool s_running;

static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;

static GameFuncs s_gameFuncs;

// This is ugly
#ifdef STARLIGHT_D3D11
static graphics::D3D11 d3d11;
#endif

CALCULATE_DELTA_TIME(CalculateDeltaTime) {
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	float deltaTime = float(currentTime.QuadPart - s_lastTime.QuadPart) / float(s_perfFreq.QuadPart);
	// Breakpoint guard
	if (deltaTime > 1.0f) {
		deltaTime = 1.0f / 60.0f;
	}
	s_lastTime = currentTime;
	return deltaTime;
}

GameFuncs LoadGameFuncs() {
	GameFuncs gameFuncs;
	ZERO_MEM(&gameFuncs, sizeof(GameFuncs));

#if _DEBUG
	HMODULE lib = LoadLibraryW(s_dllName);
	if (!lib) {
		MessageBoxW(s_hwnd, L"Could not find game .DLL!", L"Error", MB_ICONHAND);
	}
	else {
		gameFuncs.DestroyGame = (DestroyGameFunc*) GetProcAddress(lib, "DestroyGame");
		gameFuncs.UpdateGame = (UpdateGameFunc*) GetProcAddress(lib, "UpdateGame");
		gameFuncs.DestroyLogger = (DestroyLoggerFunc*) GetProcAddress(lib, "DestroyLogger");
		g_LogInfo = (LogInfoFunc*) GetProcAddress(lib, "LogInfo");
		if (gameFuncs.DestroyGame
			&& gameFuncs.UpdateGame
			&& gameFuncs.DestroyLogger
			&& g_LogInfo
			) {
			gameFuncs.valid = true;
		} else {
			MessageBoxW(s_hwnd, L"Could not load functions from .DLL!", L"Error", MB_ICONHAND);
		}
	}
#else
	g_LogInfo = logger::LogInfo;
	gameFuncs.DestroyGame = game::DestroyGame;
	gameFuncs.UpdateGame = game::UpdateGame;
	gameFuncs.DestroyLogger = logger::DestroyLogger;
	gameFuncs.valid = true;
#endif

	return gameFuncs;
}

// Try to initialize the specified rendering API.
// TODO: Remove globals for the APIs
// If it fails, it returns false.
bool LoadRenderApiImpl(EGraphicsApi e) {
	graphics::API* api = nullptr;
	switch (e) {
#ifdef STARLIGHT_D3D11
	case D3D11:
		g_LogInfo("Loading D3D11...");
		api = &d3d11;
		break;
#endif
	default:
		g_LogInfo("The requested graphics API is not enabled.");
		return false;
	}

	// Cannot init if there is no window
	assert(s_hwnd);

	PlatformData platformData;
	ZeroMemory(&platformData, sizeof(platformData));
	platformData.hWnd = s_hwnd;

#if 1
	// Init new -> destroy old
	if (api->Init(&platformData, &s_gameFuncs)) {
		if (g_renderApi) {
			g_renderApi->Destroy();
		}
		g_renderApi = api;

		// Reload fonts
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF("assets/ProggyTiny.ttf", 10.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
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

#if 0
class HeapArea
{
public:
	explicit HeapArea(std::size_t bytes) {
		start = VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	}

	~HeapArea() {
		VirtualFree(start, 0, MEM_RELEASE);
	}

	void* GetStart() { return start; }
	void* GetEnd() { return end; }

private:
	void* start;
	void* end;
};

static HeapArea* heapArea;

void* CALLBACK LibMalloc(std::size_t size) {
	return nullptr;
}

void CALLBACK LibFree(void* ptr) {

}
#endif

void ParseMessages() {
	WindowEvent message;
	// Clear queue for now
	// TODO: Process input
	while (s_queue->Dequeue(&message)) {}
}

unsigned int __stdcall MyThreadFunction(void*) {
	EGraphicsApi graphicsApi = EGraphicsApi::D3D11;
	if (!LoadRenderApiImpl(graphicsApi)) {
		s_running.store(false);
		return 1;
	}

	// Init time
	QueryPerformanceFrequency(&s_perfFreq);
	QueryPerformanceCounter(&s_lastTime);

	GameInfo gameInfo;
	ZeroMemory(&gameInfo, sizeof(gameInfo));
	gameInfo.graphicsApi = graphicsApi;
	gameInfo.imguiState = ImGui::GetCurrentContext();
	//heapArea = new HeapArea(512 * 1024 * 1024);
	//memory::SimpleArena arena(heapArea);
	//gameInfo.allocator = &arena;
	gameInfo.CalculateDeltaTime = CalculateDeltaTime;
	gameInfo.gfxFuncs = g_renderApi;

	// ENet
#if 0
	ENetCallbacks callbacks;
	ZERO_MEM(&callbacks, sizeof(callbacks));
	callbacks.malloc = memory::malloc;
	callbacks.free = memory::free;
	callbacks.no_memory = memory::no_memory;

	if (enet_initialize_with_callbacks(ENET_VERSION, &callbacks) != 0)
#endif
	if (enet_initialize() != 0)
	{
		s_running.store(false);
		return 1;
	}

	// Main loop
	while (s_running.load())
	{
		// Check if graphics API change was requested
		if (gameInfo.graphicsApi != graphicsApi) {
			if (LoadRenderApiImpl(gameInfo.graphicsApi)) {
				graphicsApi = gameInfo.graphicsApi;
				g_renderApi = gameInfo.gfxFuncs;
			} else {
				gameInfo.graphicsApi = graphicsApi;
				gameInfo.gfxFuncs = g_renderApi;
			}
		}

		ParseMessages();

		g_renderApi->ImGuiNewFrame();

		s_gameFuncs.UpdateGame(&gameInfo);

		// Rendering
		if (std::try_lock(s_mutex)) {
			g_renderApi->Render();
			s_mutex.unlock();
		}
	}

	s_gameFuncs.DestroyGame();
	g_renderApi->Destroy();
	ImGui::Shutdown();
	g_renderApi = nullptr;

	//delete heapArea;

	enet_deinitialize();

	_endthreadex(0);
	return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	WindowEvent params;
	ZeroMemory(&params, sizeof(params));
	params.hWnd = hWnd;
	params.msg = msg;
	params.wParam = wParam;
	params.lParam = lParam;

	s_queue->Enqueue(params);

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

int CALLBACK WinMain(
	HINSTANCE   hInstance,
	HINSTANCE   hPrevInstance,
	LPSTR       lpCmdLine,
	int         nCmdShow)
{
	//std::set_new_handler(memory::no_memory);

	s_gameFuncs = LoadGameFuncs();
	if (!s_gameFuncs.valid) {
		return 1;
	}

	ImGuiIO& io = ImGui::GetIO();
	//io.MemAllocFn = memory::malloc;
	//io.MemFreeFn = memory::free;

	//_crtBreakAlloc = 4015;
	s_queue = new util::ThreadSafeQueue<WindowEvent>();

	g_LogInfo(SL_BUILD_DATE);

	// Log CPU features
	{
		struct CPUInfo {
			union {
				int i[4];
			};
		} info = { 0 };

		// Get number of functions
		__cpuid(info.i, 0);
		int nIds_ = info.i[0];

		// Dump info
		std::vector<CPUInfo> data;
		for (int i = 0; i <= nIds_; ++i)
		{
			__cpuidex(info.i, i, 0);
			data.push_back(info);
		}

		// Vendor
		if(data.size() >= 0) {
			char vendor[13] = {0};
			memcpy(vendor + 0, data[0].i + 1, 4);
			memcpy(vendor + 4, data[0].i + 3, 4);
			memcpy(vendor + 8, data[0].i + 2, 4);
			g_LogInfo("CPU Vendor: " + std::string(vendor));
		}
	}

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

	assert(CoInitializeEx(NULL, COINIT_MULTITHREADED) == S_OK);

	// Create thread
	s_running.store(true);
	unsigned int threadID;
	HANDLE thread = (HANDLE) _beginthreadex(nullptr, 0, MyThreadFunction, nullptr, 0, &threadID);
	if (!thread) {
		// Error creating thread
		return 1;
	}

	// Message loop
	MSG msg;
	while (s_running.load() && GetMessageW(&msg, nullptr, 0, 0))
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);

	s_gameFuncs.DestroyLogger();

	io.Fonts->Clear();

	delete s_queue;

	UnregisterClassW(className, GetModuleHandleW(nullptr));

	CoUninitialize();

	_CrtDumpMemoryLeaks();

	return 0;
}

#if 0

// Global memory functions

void* MEM_CALL operator new(std::size_t n) throw() {
	return memory::malloc(n);
}

void* MEM_CALL operator new[](std::size_t s) throw() {
	return operator new(s);
}

void MEM_CALL operator delete(void * p) throw() {
	return memory::free(p);
}

void MEM_CALL operator delete[](void *p) throw() {
	return operator delete(p);
}

// Wrappers

void* MEM_CALL memory::malloc(std::size_t size) {
	return ::malloc(size);
}

void MEM_CALL memory::free(void* ptr) {
	return ::free(ptr);
}

void* MEM_CALL memory::realloc(void* ptr, std::size_t size) {
	return ::realloc(ptr, size);
}

void MEM_CALL memory::no_memory() {

}
#endif
