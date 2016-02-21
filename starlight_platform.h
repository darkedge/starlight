#pragma once
#include "starlight_glm.h"

// This is the default calling convention
// Does not have to be specified manually
#define SL_CALLBACK __vectorcall

// Probably needed when implementing dynamic game code loading
#if 0
#ifdef SL_DLL
#ifdef SL_BUILDING_LIB
#define SL_API __declspec( dllexport )
#else
#define SL_API __declspec( dllimport )
#endif /* SL_BUILDING_LIB */
#else /* !SL_DLL */
#define SL_API extern
#endif /* SL_DLL */
#endif

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
