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
PROTOBUF := $(EXTERNAL)/protobuf-2.6.1
VECTORMATH := $(EXTERNAL)/sce_vectormath-master

slBasicCompile := /nologo /Gm- /MDd /GR- /EHs-c- /fp:fast /fp:except- /Oi
slDefinitions := -DSL_CL -D_DEBUG -DUNICODE -D_UNICODE -D_SCL_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=0 -DNOMINMAX -DVC_EXTRALEAN -DSTRICT -DWIN32_LEAN_AND_MEAN -DNOGDI -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS
slInclude := $(addprefix /I, . $(INC) $(VECTORMATH)\include $(ENET)\include $(IMGUI) $(PROTOBUF)\src)
slCompile := $(slBasicCompile) $(slDefinitions) $(slInclude) /Ob1 /WX /W4 /wd4100 /wd4505 /Z7

# [Linker options]
# /time: display timings (undocumented)

slLink := /incremental:no /LIBPATH:$(BUILD)/lib enet.lib imgui.lib winmm.lib ws2_32.lib dxguid.lib ole32.lib

all: starlight.dll starlight.exe assets

assets:
	xcopy $(IMGUI)\extra_fonts\\*.ttf $(ASSETS) /Y /Q

enet.lib: $(ENET)/*.c
	if not exist $(BUILD)\enet mkdir $(BUILD)\enet
	cl /c $(slBasicCompile) /Fo$(BUILD)/enet/ /I../external/enet-1.3.13/include ../external/enet-1.3.13/*.c
	lib /nologo /out:enet.lib $(BUILD)/enet/*.obj

imgui.lib: $(addprefix $(IMGUI)/,imgui.cpp imgui_demo.cpp imgui_draw.cpp examples/directx11_example/imgui_impl_dx11.cpp)
	if not exist $(BUILD)\imgui mkdir $(BUILD)\imgui
	cl /c $(slBasicCompile) /Fo$(BUILD)/imgui/ /I$(IMGUI) $^
	lib /nologo /out:imgui.lib $(BUILD)/imgui/*.obj

starlight_hlsl_generated.h: hlsl_codegen.lua
	lua $^

ModdingApi.class: ../modding-api/src/ModdingApi.java
	javac -d . $^

starlight_java_generated.h: ModdingApi.class
	javah -o starlight_java_generated.h ModdingApi

starlight.dll: enet.lib imgui.lib ../starlight/*.cpp $(INC)/*.h starlight_java_generated.h
	if not exist $(BUILD)\bin mkdir $(BUILD)\bin
	cl $(slCompile) /I"$(JAVA_HOME)\include" /I"$(JAVA_HOME)\include\win32" /Fo$(BUILD)/bin/ /LD /Festarlight.dll ../starlight/starlight_ub.cpp /link /libpath:"$(JAVA_HOME)\lib" $(slLink) jvm.lib Delayimp.lib /PDB:$(BUILD)/bin/starlight_$(SL_RANDOM).pdb /delayload:jvm.dll

SimplePixelShader.h: SimplePixelShader.hlsl
	fxc /Zi /E"main" /Od /Vn"g_SimplePixelShader" /WX /T ps_4_0 /Fh"SimplePixelShader.h" /nologo SimplePixelShader.hlsl

SimpleVertexShader.h: SimpleVertexShader.hlsl starlight_hlsl_generated.h
	fxc /I. /Zi /E"main" /Od /Vn"g_SimpleVertexShader" /WX /T vs_4_0 /Fh"SimpleVertexShader.h" /nologo SimpleVertexShader.hlsl

starlight.exe: enet.lib imgui.lib *.cpp SimplePixelShader.h SimpleVertexShader.h $(INC)/*.h
	if not exist $(BUILD)\bin mkdir $(BUILD)\bin
	cl $(slCompile) /Fo$(BUILD)/bin/ /Festarlight.exe /I$(IMGUI)/examples/directx11_example starlight_win32_ub.cpp /link $(slLink) d3dcompiler.lib d3d11.lib Advapi32.lib

clean:
	del $(ASSETS)\*.ttf
	del *.class
	del *.h
	del *.log
	del enet.lib
	del imgui.ini
	del imgui.lib
	del SimplePixelShader.h
	del SimpleVertexShader.h
	del starlight.dll
	del starlight.exe
	del starlight.exp
	del starlight.lib
	del starlight.pdb
	del starlight_temp.dll
	if exist $(BUILD) rmdir /s /q $(BUILD)
