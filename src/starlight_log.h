#pragma once
#include <string>

// NOTE: Platform layer can't use any of these (linker errors!)
namespace logger {
	extern "C"
	__declspec(dllexport)
	void __cdecl InitLogger();

	extern "C"
	__declspec(dllexport)
	void __cdecl LogInfo(const std::string& str);

	void Render();

	extern "C"
	__declspec(dllexport)
	void __cdecl DestroyLogger();
}

// Pulled out of the namespace for convenience
using logger::LogInfo;
