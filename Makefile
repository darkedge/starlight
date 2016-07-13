# [Compiler options]
# /nologo: suppress copyright message
# /Gm-: Disable minimal rebuild
# /MDd: Runtime library: Multi-threaded Debug DLL
# /GR-: disable RTTI
# /EHs-c-: disable exceptions
# /fp:fast: "fast" floating-point model; results are less predictable
# /fp:except-: disable floating-point exceptions when generating code
# /Oi: enable intrinsic functions
# /Ob1: only __inline function expansion
# /WX: treat warnings as errors
# /W4: warning level 4
# /wd4100:
# /wd4505:
# /Bt: display timings (undocumented)

slBasicCompile=/nologo /Gm- /MDd /GR- /EHs-c- /fp:fast /fp:except- /Oi
slDefinitions=-DSL_CL -D_DEBUG -DUNICODE -D_UNICODE -D_SCL_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=0 -DNOMINMAX -DVC_EXTRALEAN -DSTRICT -DWIN32_LEAN_AND_MEAN -DNOGDI -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS
slInclude=/Iinclude /Iexternal/sce_vectormath-master/include /Iexternal/flatbuffers-1.3.0/include /Iexternal/protobuf-2.6.1/src /Iexternal/enet-1.3.13/include /Iexternal/imgui-1.49
slCompile=$(slBasicCompile) $(slDefinitions) $(slInclude) /Ob1 /WX /W4 /wd4100 /wd4505 /Z7

# [Linker options]
# /time: display timings (undocumented)

slLink=/incremental:no /LIBPATH:build/lib enet.lib imgui.lib winmm.lib ws2_32.lib dxguid.lib ole32.lib

all: starlight.dll starlight.exe asset

asset:
	@xcopy external\imgui-1.49\extra_fonts\*.ttf assets /Y /Q > NUL

enet.lib: external/enet-1.3.13/*.c
	@if not exist build\enet mkdir build\enet
	@cl /c $(slBasicCompile) /Fobuild/enet/ /Iexternal/enet-1.3.13/include external/enet-1.3.13/*.c
	@lib /nologo /out:enet.lib build/enet/*.obj

imgui.lib: external/imgui-1.49/imgui.cpp external/imgui-1.49/imgui_demo.cpp external/imgui-1.49/imgui_draw.cpp external/imgui-1.49/examples/directx11_example/imgui_impl_dx11.cpp
	@if not exist build\imgui mkdir build\imgui
	@cl /c $(slBasicCompile) /Fobuild/imgui/ /Iexternal/imgui-1.49/ external/imgui-1.49/imgui.cpp external/imgui-1.49/imgui_demo.cpp external/imgui-1.49/imgui_draw.cpp external/imgui-1.49/examples/directx11_example/imgui_impl_dx11.cpp
	@lib /nologo /out:imgui.lib build/imgui/*.obj

starlight_hlsl_generated.h: win32/hlsl_codegen.lua
	@lua win32/hlsl_codegen.lua

starlight.dll: enet.lib imgui.lib starlight/*.cpp include/*.h
	@if not exist build\bin mkdir build\bin
	@cl $(slCompile) /Fobuild/bin/ /LD /Festarlight.dll starlight/starlight_ub.cpp /link $(slLink) /PDB:build/bin/starlight_$(SL_RANDOM).pdb

SimplePixelShader.h: win32/SimplePixelShader.hlsl
	@fxc /Zi /E"main" /Od /Vn"g_SimplePixelShader" /WX /T ps_4_0 /Fh"SimplePixelShader.h" /nologo win32/SimplePixelShader.hlsl

SimpleVertexShader.h: win32/SimpleVertexShader.hlsl starlight_hlsl_generated.h
	@fxc /I. /Zi /E"main" /Od /Vn"g_SimpleVertexShader" /WX /T vs_4_0 /Fh"SimpleVertexShader.h" /nologo win32/SimpleVertexShader.hlsl

starlight.exe: enet.lib imgui.lib win32/*.cpp SimplePixelShader.h SimpleVertexShader.h include/*.h
	@if not exist build\bin mkdir build\bin
	@cl $(slCompile) /Fobuild/bin/ /Festarlight.exe /Iexternal/imgui-1.49/examples/directx11_example win32/starlight_win32_ub.cpp /link $(slLink) d3dcompiler.lib d3d11.lib
