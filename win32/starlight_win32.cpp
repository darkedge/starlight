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

#include "starlight_renderer_windows.h"
#include "starlight_graphics.h"
#include "starlight_game.h"
#include "mj_controls.h"
#include <imgui.h>
#include <atomic>
#include "starlight_thread_safe_queue.h"
#include "starlight_log.h"

#include <process.h> // _beginthreadex
#include <Psapi.h> // GetProcessMemoryInfo

static const char* s_dllName = "starlight.dll";

// Globals
static graphics::API* g_renderApi;

static HWND s_hwnd = nullptr;
static util::ThreadSafeQueue<WindowEvent>* s_queue;
static std::atomic_bool s_running;

static LARGE_INTEGER s_lastTime;
static LARGE_INTEGER s_perfFreq;

#ifdef _DEBUG
static LARGE_INTEGER s_lastDLLLoadTime;
#endif

// Maybe separate struct for logger etc?
struct GameFuncs {
    DestroyLoggerFunc* DestroyLogger;
    UpdateGameFunc* UpdateGame;
    DestroyGameFunc* DestroyGame;
    bool valid;

    FILETIME DLLLastWriteTime;
    HMODULE dll;
};

static GameFuncs s_gameFuncs;
static HardwareInfo s_hardware;
static MJControls s_controls;

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

struct FunctionCall {
    GameThread* func;
    void* args;
    HANDLE confirmation;
};

unsigned int __stdcall slThreadProc(void* lpParameter) {
    // Copy 
    FunctionCall call = *((FunctionCall*)lpParameter);

    // Signal that we copied the function call
    SetEvent(call.confirmation);

    // Call function
    call.func(call.args);

    return S_OK;
}

CREATE_THREAD(slCreateThread);
void* slCreateThread(GameThread* func, void* args) {
    // We wrap the arguments in a struct to avoid specifying a calling convention
    FunctionCall call = { 0 };
    call.func = func;
    call.args = args;
    call.confirmation = CreateEventA(NULL, FALSE, FALSE, NULL);
    assert(call.confirmation);

    // Start the thread
    unsigned threadID; // TODO: Use this?
    HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, slThreadProc, &call, 0, &threadID);
    assert(thread);

    // Wait for the function call to be copied to the thread
    DWORD result = WaitForSingleObject(call.confirmation, INFINITE);
    assert(result != WAIT_FAILED);
    CloseHandle(call.confirmation);

    // Return thread as void*
    return thread;
}

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return(LastWriteTime);
}

GameFuncs LoadGameFuncs() {
    GameFuncs gameFuncs = { 0 };

#if _DEBUG
    gameFuncs.DLLLastWriteTime = Win32GetLastWriteTime((char*)s_dllName);
    QueryPerformanceCounter(&s_lastDLLLoadTime);

    if (!CopyFileA(s_dllName, "starlight_temp.dll", FALSE)) {
        MessageBoxA(s_hwnd, "Could not write to starlight_temp.dll!", "Error", MB_ICONHAND);
    }

    gameFuncs.dll = LoadLibraryA("starlight_temp.dll");
    if (!gameFuncs.dll) {
        MessageBoxA(s_hwnd, "Could not find game .DLL!", "Error", MB_ICONHAND);
    }
    else {
        gameFuncs.DestroyGame = (DestroyGameFunc*) GetProcAddress(gameFuncs.dll, "DestroyGame");
        gameFuncs.UpdateGame = (UpdateGameFunc*) GetProcAddress(gameFuncs.dll, "UpdateGame");
        gameFuncs.DestroyLogger = (DestroyLoggerFunc*) GetProcAddress(gameFuncs.dll, "DestroyLogger");
        g_LogInfo = (LogInfoFunc*) GetProcAddress(gameFuncs.dll, "LogInfo");
        if (gameFuncs.DestroyGame
            && gameFuncs.UpdateGame
            && gameFuncs.DestroyLogger
            && g_LogInfo
            ) {
            gameFuncs.valid = true;
        } else {
            MessageBoxA(s_hwnd, "Could not load functions from .DLL!", "Error", MB_ICONHAND);
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
        io.Fonts->AddFontFromFileTTF("../assets/ProggyTiny.ttf", 10.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
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
    gameInfo.controls = &s_controls;
    gameInfo.CreateThread = slCreateThread;

    // Hardware info
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        s_hardware.numLogicalThreads = (uint32_t) sysinfo.dwNumberOfProcessors;
    }
    
    gameInfo.hardware = &s_hardware;

    // ENet
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
        s_controls.BeginFrame();

        s_gameFuncs.UpdateGame(&gameInfo);

        s_controls.EndFrame();

#ifdef _DEBUG
        // Reload DLL
        FILETIME NewDLLWriteTime = Win32GetLastWriteTime((char*)s_dllName);

        // If the .DLL is being written to, the last write time may change multiple times
        // So we need to add a cooldown
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        if (CompareFileTime(&NewDLLWriteTime, &s_gameFuncs.DLLLastWriteTime) != 0
            && (currentTime.QuadPart - s_lastDLLLoadTime.QuadPart) / s_perfFreq.QuadPart >= 1) {

            if (s_gameFuncs.dll) {
                FreeLibrary(s_gameFuncs.dll);
            }

            s_gameFuncs = LoadGameFuncs();
            g_LogInfo("Reloaded DLL.");
        } else {
            // Ignore changes
            s_gameFuncs.DLLLastWriteTime = NewDLLWriteTime;
        }
#endif

        // Rendering
        g_renderApi->Render();
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
            return 0;
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
    s_gameFuncs = LoadGameFuncs();
    assert(s_gameFuncs.valid);

    ImGuiIO& io = ImGui::GetIO();
    //io.MemAllocFn = memory::malloc;
    //io.MemFreeFn = memory::free;

    //_crtBreakAlloc = 4015;
    s_queue = new util::ThreadSafeQueue<WindowEvent>();

    g_LogInfo(SL_BUILD_DATE);

    // Log CPU features
    {
#if 0
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
#endif
        // http://stackoverflow.com/questions/850774/how-to-determine-the-hardware-cpu-and-ram-on-a-machine
        int CPUInfo[4] = { 0 };
        char CPUBrandString[0x40];
        // Get the information associated with each extended ID.
        __cpuid(CPUInfo, 0x80000000);
        int nExIds = CPUInfo[0];
        for (int i = 0x80000000; i <= nExIds; i++)
        {
            __cpuid(CPUInfo, i);
            // Interpret CPU brand string
            if  (i == 0x80000002)
                memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
            else if  (i == 0x80000003)
                memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
            else if  (i == 0x80000004)
                memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        }
        //string includes manufacturer, model and clockspeed
        std::string brand(CPUBrandString);
        g_LogInfo(brand.substr(brand.find_first_not_of(" ")));

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        g_LogInfo("Logical processors: " + std::to_string(sysInfo.dwNumberOfProcessors));

        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof (statex);
        GlobalMemoryStatusEx(&statex);
        g_LogInfo("Total System Memory: " + std::to_string(statex.ullTotalPhys/1024/1024) + " MB");
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
    LONG windowWidth = windowRect.right - windowRect.left;
    LONG windowHeight = windowRect.bottom - windowRect.top;
    LONG x = desktopRect.right / 2 - windowWidth / 2;
    LONG y = desktopRect.bottom / 2 - windowHeight / 2;

#ifdef _DEBUG
    // Move the screen to the right monitor on JOTARO
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(computerName);
    GetComputerNameW(computerName, &dwSize);
    if (wcscmp(computerName, L"JOTARO") == 0) {
        x += 1920;
    }
#endif

    s_hwnd = CreateWindowExW(
        0L,
        className,
        L"Starlight",
        WS_OVERLAPPEDWINDOW,
        x,
        y,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
        );

    // Show the window
    ShowWindow(s_hwnd, SW_MAXIMIZE);
    UpdateWindow(s_hwnd);

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
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    s_gameFuncs.DestroyLogger();

    io.Fonts->Clear();

    delete s_queue;

    UnregisterClassW(className, GetModuleHandleW(nullptr));

    _CrtDumpMemoryLeaks();

    return 0;
}
