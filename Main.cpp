#include "starlight_windows.h"
#include <Windows.h>

typedef void(*Start)();

int __cdecl main() {
	HMODULE lib = LoadLibraryW(L"starlight.dll");
	if (!lib) {
		GetLastError();
	} else {
		Start start = (Start) GetProcAddress(lib, "Start");
		if (!start) {
			GetLastError();
		} else {
			start();
		}
	}
}
