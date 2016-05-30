:: This file generates debug unity builds for rapid iteration

@echo off
pushd %~dp0
if not exist build mkdir build
if not exist build\enet mkdir build\enet
if not exist build\imgui mkdir build\imgui
if not exist build\lib mkdir build\lib
if not exist build\bin mkdir build\bin

:: [Compiler options]
:: /nologo: suppress copyright message
:: /Gm-: Disable minimal rebuild
:: /MDd: Runtime library: Multi-threaded Debug DLL
:: /GR-: disable RTTI
:: /EHs-c-: disable exceptions
:: /fp:fast: "fast" floating-point model; results are less predictable
:: /fp:except-: disable floating-point exceptions when generating code
:: /Oi: enable intrinsic functions
:: /Ob1: only __inline function expansion
:: /WX: treat warnings as errors
:: /W4: warning level 4
:: /wd4100:
:: /wd4505:
:: /Bt: display timings (undocumented)

set slBasicCompile=/nologo /Gm- /MDd /GR- /EHs-c- /fp:fast /fp:except- /Oi
set slDefinitions=-DSL_CL -D_DEBUG -DUNICODE -D_UNICODE -D_SCL_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=0 -DNOMINMAX -DVC_EXTRALEAN -DSTRICT -DWIN32_LEAN_AND_MEAN -DNOGDI -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS
set slInclude=/I..\..\src /I..\..\external\sce_vectormath-master\include /I..\..\external\flatbuffers-1.3.0\include /I..\..\external\protobuf-2.6.1\src /I..\..\external\enet-1.3.13\include /I..\..\external\imgui-1.49
set slCompile=%slBasicCompile% %slDefinitions% %slInclude% /Ob1 /WX /W4 /wd4100 /wd4505 /Z7 /Bt
::set slFxCompile=/Zi /E"main" /Od /Vn"g_%(Filename)" /WX /Fh"src/%(Filename).h" /nologo

:: [Linker options]
:: /time: display timings (undocumented)

set slLink=/time /incremental:no /LIBPATH:../lib enet.lib imgui.lib winmm.lib ws2_32.lib dxguid.lib

:cook
if exist build\bin\assets goto enet
call cook build\bin\assets

:enet
if exist build\lib\enet.lib goto imgui
echo. && echo [ENet]
pushd external\enet-1.3.13
cl /c %slBasicCompile% /Fo..\..\build\enet\ /Iinclude *.c
popd
pushd build\enet
lib /nologo /out:..\lib\enet.lib *.obj
popd

:imgui
if exist build\lib\imgui.lib goto starlight
echo. && echo [ImGui]
pushd external\imgui-1.49
cl /c %slBasicCompile% /Fo..\..\build\imgui\ /I. imgui.cpp imgui_demo.cpp imgui_draw.cpp examples\directx11_example\imgui_impl_dx11.cpp
popd
pushd build\imgui
lib /nologo /out:..\lib\imgui.lib *.obj
popd

:starlight
echo. && echo [starlight_ub]
pushd build\bin
cl %slCompile% /LD /Festarlight.dll ..\..\src\starlight_ub.cpp /link %slLink% /PDB:starlight_%random%.pdb

:starlight_win32
echo. && echo [starlight_win32_ub]
fxc /Zi /E"main" /Od /Vn"g_SimplePixelShader" /WX /T ps_4_0 /Fh"..\..\src/SimplePixelShader.h" /nologo ..\..\src/SimplePixelShader.hlsl
fxc /Zi /E"main" /Od /Vn"g_SimpleVertexShader" /WX /T vs_4_0 /Fh"..\..\src/SimpleVertexShader.h" /nologo ..\..\src/SimpleVertexShader.hlsl
cl %slCompile% /Festarlight.exe /I..\..\external\imgui-1.49\examples\directx11_example ..\..\src\starlight_win32_ub.cpp /link %slLink% d3dcompiler.lib d3d11.lib
popd
popd