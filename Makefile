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
slInclude=/Isrc /Iexternal/sce_vectormath-master/include /Iexternal/flatbuffers-1.3.0/include /Iexternal/protobuf-2.6.1/src /Iexternal/enet-1.3.13/include /Iexternal/imgui-1.49
slCompile=$(slBasicCompile) $(slDefinitions) $(slInclude) /Ob1 /WX /W4 /wd4100 /wd4505 /Z7 /Bt

# [Linker options]
# /time: display timings (undocumented)

slLink=/time /incremental:no /LIBPATH:build/lib enet.lib imgui.lib winmm.lib ws2_32.lib dxguid.lib ole32.lib

all: starlight starlight_win32

folder:
	if not exist build\lib mkdir build\lib

enet: folder
	cl /c $(slBasicCompile) /Fobuild/enet/ /Iexternal/enet-1.3.13/include external/enet-1.3.13/*.c
	lib /nologo /out:build/lib/enet.lib build/enet/*.obj

imgui: folder
	cl /c $(slBasicCompile) /Fobuild/imgui/ /Iexternal/imgui-1.49/ external/imgui-1.49/imgui.cpp external/imgui-1.49/imgui_demo.cpp external/imgui-1.49/imgui_draw.cpp external/imgui-1.49/examples/directx11_example/imgui_impl_dx11.cpp
	lib /nologo /out:build/lib/imgui.lib build/imgui/*.obj

starlight: enet imgui
	cl $(slCompile) /LD /Festarlight.dll src/starlight_ub.cpp /link $(slLink) /PDB:starlight_$(SL_RANDOM).pdb

starlight_win32: enet imgui
	fxc /Zi /E"main" /Od /Vn"g_SimplePixelShader" /WX /T ps_4_0 /Fh"src/SimplePixelShader.h" /nologo src/SimplePixelShader.hlsl
	fxc /Zi /E"main" /Od /Vn"g_SimpleVertexShader" /WX /T vs_4_0 /Fh"src/SimpleVertexShader.h" /nologo src/SimpleVertexShader.hlsl
	cl $(slCompile) /Festarlight.exe /Iexternal/imgui-1.49/examples/directx11_example src/starlight_win32_ub.cpp /link $(slLink) d3dcompiler.lib d3d11.lib
