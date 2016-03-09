#pragma once
#ifdef _WIN32
#include <Windows.h>
// Stuff in this header is shared between the windows-specific code
// and all supported renderers

struct PlatformData {
	HWND hWnd;
};

// For Windows, these are the WndProc parameters
struct WindowEvent {
	HWND hWnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
};

#endif
