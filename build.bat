@echo off
pushd %~dp0
if not exist build mkdir build
if not exist build\enet mkdir build\enet
if not exist build\imgui mkdir build\imgui
if not exist build\lib mkdir build\lib
if not exist build\bin mkdir build\bin

:: Build ENet
echo [ENet]
pushd external\enet-1.3.13
cl /c /nologo /Fo..\..\build\enet\ /Iinclude callbacks.c compress.c host.c list.c packet.c peer.c protocol.c unix.c win32.c
popd
pushd build\enet
lib /nologo /out:..\lib\enet.lib callbacks.obj compress.obj host.obj list.obj packet.obj peer.obj protocol.obj unix.obj win32.obj
popd

:: Build ImGui
echo [ImGui]
pushd external\imgui-1.47
cl /c /nologo /Fo..\..\build\imgui\ /I. imgui.cpp imgui_demo.cpp imgui_draw.cpp examples\directx11_example\imgui_impl_dx11.cpp
popd
pushd build\imgui
lib /nologo /out:..\lib\imgui.lib imgui.obj imgui_demo.obj imgui_draw.obj imgui_impl_dx11.obj
popd

:: Build starlight
echo [starlight_ub]
cl /nologo /Fobuild\bin\starlight.dll /Isrc src/starlight_ub.cpp /LD /link /PDB:starlight_%random%.pdb

:: Build starlight_win32
echo [starlight_win32_ub]
cl /nologo /Isrc src/starlight_win32_ub.cpp /link

popd