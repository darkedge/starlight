#pragma once
#include "starlight_glm.h"

namespace platform {
	glm::ivec2 GetWindowSize();
}

#ifdef _WIN32
//#include "starlight_windows.h"
#include <Windows.h>
namespace platform {
	HWND GetHWND();
}
#endif
