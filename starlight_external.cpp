// Unity build file for external libraries

// Wrapper header for Windows to prevent warning spam
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4245 4456 4701)
#endif

// ENet
#include "external/enet-1.3.13/callbacks.c"
#include "external/enet-1.3.13/compress.c"
#include "external/enet-1.3.13/host.c"
#include "external/enet-1.3.13/list.c"
#include "external/enet-1.3.13/packet.c"
#include "external/enet-1.3.13/peer.c"
#include "external/enet-1.3.13/protocol.c"
#include "external/enet-1.3.13/unix.c"
#include "external/enet-1.3.13/win32.c"

// ImGui
#include "imgui.cpp"
#include "imgui_draw.cpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
