# $^	The names of all the prerequisites, with spaces between them.
# $@	The file name of the target of the rule.
# $?	The names of all the prerequisites that are newer than the target
# $(INCLUDE) contains MSVC's default include paths, separated by semicolons

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

ASSETS := ..\assets
BUILD := build
EXTERNAL := ..\external
INC := ..\include

ENET := $(EXTERNAL)\enet-1.3.13
IMGUI := $(EXTERNAL)\imgui-1.49
LUA := $(EXTERNAL)\LuaJIT-2.0.4\src
PROTOBUF := $(EXTERNAL)/protobuf-2.6.1
VECTORMATH := $(EXTERNAL)/sce_vectormath-master

slBasicCompile := /nologo /Gm- /MDd /GR- /EHs-c- /fp:fast /fp:except- /Oi
slDefinitions := -DSL_CL -D_DEBUG -DUNICODE -D_UNICODE -D_SCL_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=0 -DNOMINMAX -DVC_EXTRALEAN -DSTRICT -DWIN32_LEAN_AND_MEAN -DNOGDI -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS
slInclude := $(addprefix /I, . $(INC) $(VECTORMATH)\include $(ENET)\include $(IMGUI) $(PROTOBUF)\src $(LUA))
slCompile := $(slBasicCompile) $(slDefinitions) $(slInclude) /Ob1 /WX /W4 /wd4100 /wd4505 /Z7

# [Linker options]
# /time: display timings (undocumented)

slLink := /incremental:no /LIBPATH:$(BUILD)/lib enet.lib imgui.lib lua51.lib winmm.lib ws2_32.lib dxguid.lib ole32.lib

all: starlight.dll starlight.exe assets

assets:
	xcopy $(IMGUI)\extra_fonts\\*.ttf $(ASSETS) /Y /Q

starlight_hlsl_generated.h: hlsl_codegen.lua
	luajit hlsl_codegen.lua

starlight.dll: ../starlight/*.cpp $(INC)/*.h
	if not exist $(BUILD)\bin mkdir $(BUILD)\bin
	cl $(slCompile) /Fo$(BUILD)/bin/ /LD /Festarlight.dll ../starlight/starlight_ub.cpp /link $(slLink) /PDB:$(BUILD)/bin/starlight_$(SL_RANDOM).pdb

SimplePixelShader.h: SimplePixelShader.hlsl
	fxc /Zi /E"main" /Od /Vn"g_SimplePixelShader" /WX /T ps_4_0 /Fh"SimplePixelShader.h" /nologo SimplePixelShader.hlsl

SimpleVertexShader.h: SimpleVertexShader.hlsl starlight_hlsl_generated.h
	fxc /I. /Zi /E"main" /Od /Vn"g_SimpleVertexShader" /WX /T vs_4_0 /Fh"SimpleVertexShader.h" /nologo SimpleVertexShader.hlsl

starlight.exe: *.cpp SimplePixelShader.h SimpleVertexShader.h $(INC)/*.h
	if not exist $(BUILD)\bin mkdir $(BUILD)\bin
	cl $(slCompile) /Fo$(BUILD)/bin/ /Fe$@ /I$(IMGUI)/examples/directx11_example starlight_win32_ub.cpp /link $(slLink) d3dcompiler.lib d3d11.lib Advapi32.lib

clean:
	del $(ASSETS)\*.ttf
	del *.h
	del *.log
	del enet.lib
#	del imgui.ini
	del imgui.lib
	del log.txt
	del lua.exe
	del lua.lib
	del luac.exe
	del SimplePixelShader.h
	del SimpleVertexShader.h
	del starlight.dll
	del starlight.exe
	del starlight.exp
	del starlight.lib
	del starlight.pdb
	del starlight_temp.dll
	if exist $(BUILD) rmdir /s /q $(BUILD)
