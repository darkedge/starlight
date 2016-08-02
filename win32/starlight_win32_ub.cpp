#define SL_UB
// starlight_win32.cpp defines SL_IMPL so include that first
#include "starlight_win32.cpp"
#include "starlight_d3d11.cpp"
#include "starlight_d3d11_imgui.cpp"

#pragma warning(push)
#pragma warning(disable:4244)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)
