#pragma once
#include "starlight.h"
#include <vectormath/scalar/cpp/vectormath_aos.h>
#include "starlight_config.h"
#include <cstdint>

// Platform needs to provide this
struct PlatformData;
struct WindowEvent;
struct GameFuncs;
struct TempMesh;

enum EGraphicsApi {
	D3D12,
	D3D11,
	D3D10,
	OpenGL,
	Vulkan,
	Metal,
};

// Note: #undef these at the bottom of this file
#define FUNC_00 void SL_CALL Destroy()
#define FUNC_01 void SL_CALL Render()
#define FUNC_02 void SL_CALL Resize(int32_t width, int32_t height)
#define FUNC_03 bool SL_CALL Init(PlatformData* data, GameFuncs* funcs)
#define FUNC_04 void SL_CALL Update()
#define FUNC_05 void SL_CALL ImGuiNewFrame()
#define FUNC_06 bool SL_CALL ImGuiHandleEvent(WindowEvent* event)
#define FUNC_07 int32_t SL_CALL AddChunk(TempMesh* mesh)
#define FUNC_08 void SL_CALL SetPlayerCameraViewMatrix(Vectormath::Aos::Matrix4 mat)
#define FUNC_09 void SL_CALL SetProjectionMatrix(Vectormath::Aos::Matrix4 mat)

#define PURE_VIRTUAL(_f) virtual _f = 0
#define OVERRIDE_FINAL(_f) virtual _f override final

#define RENDERER_IMPLEMENTATION(_c) \
class _c : public API { \
public: \
	OVERRIDE_FINAL(FUNC_00); \
	OVERRIDE_FINAL(FUNC_01); \
	OVERRIDE_FINAL(FUNC_02); \
	OVERRIDE_FINAL(FUNC_03); \
	OVERRIDE_FINAL(FUNC_04); \
	OVERRIDE_FINAL(FUNC_05); \
	OVERRIDE_FINAL(FUNC_06); \
	OVERRIDE_FINAL(FUNC_07); \
	OVERRIDE_FINAL(FUNC_08); \
	OVERRIDE_FINAL(FUNC_09); \
};

namespace graphics {

	class API {
	public:
		PURE_VIRTUAL(FUNC_00);
		PURE_VIRTUAL(FUNC_01);
		PURE_VIRTUAL(FUNC_02);
		PURE_VIRTUAL(FUNC_03);
		PURE_VIRTUAL(FUNC_04);
		PURE_VIRTUAL(FUNC_05);
		PURE_VIRTUAL(FUNC_06);
		PURE_VIRTUAL(FUNC_07);
		PURE_VIRTUAL(FUNC_08);
		PURE_VIRTUAL(FUNC_09);
	};


#ifdef STARLIGHT_D3D10
#define D3D10_IMPL(_c) RENDERER_IMPLEMENTATION(_c)
#else
#define D3D10_IMPL(_c)
#endif

#ifdef STARLIGHT_D3D11
#define D3D11_IMPL(_c) RENDERER_IMPLEMENTATION(_c)
#else
#define D3D11_IMPL(_c)
#endif

#ifdef STARLIGHT_D3D12
#define D3D12_IMPL(_c) RENDERER_IMPLEMENTATION(_c)
#else
#define D3D12_IMPL(_c)
#endif

#ifdef STARLIGHT_VULKAN
#define VULKAN_IMPL(_c) RENDERER_IMPLEMENTATION(_c)
#else
#define VULKAN_IMPL(_c)
#endif

#ifdef STARLIGHT_OPENGL
#define OPENGL_IMPL(_c) RENDERER_IMPLEMENTATION(_c)
#else
#define OPENGL_IMPL(_c)
#endif

#ifdef STARLIGHT_METAL
#define METAL_IMPL(_c) RENDERER_IMPLEMENTATION(_c)
#else
#define METAL_IMPL(_c)
#endif


// Support listing
#ifdef _WIN32
	D3D10_IMPL(D3D10);
	D3D11_IMPL(D3D11);
	D3D12_IMPL(D3D12);
	VULKAN_IMPL(Vulkan);
	OPENGL_IMPL(OpenGL);
#endif

#ifdef __APPLE__
	OPENGL_IMPL(OpenGL);
	METAL_IMPL(Metal);
#endif

#ifdef __linux__
	OPENGL_IMPL(OpenGL);
	VULKAN_IMPL(Vulkan);
#endif

} // namespace renderer

#undef RENDERER_IMPLEMENTATION

#undef OVERRIDE_FINAL
#undef PURE_VIRTUAL

#undef FUNC_09
#undef FUNC_08
#undef FUNC_07
#undef FUNC_06
#undef FUNC_05
#undef FUNC_04
#undef FUNC_03
#undef FUNC_02
#undef FUNC_01
#undef FUNC_00
