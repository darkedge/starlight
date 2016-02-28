#pragma once

#ifdef _WIN32
template<typename T>
inline void SafeRelease(T& ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}
#endif

#ifdef _DEBUG
#include <sstream>
#define STR(x) #x
#define XSTR(x) STR(x)
#define D3D_TRY(expr) \
do { \
	HRESULT	hr = expr; \
	if (FAILED( hr )) { \
		std::stringstream s;\
		s << __FILE__ << "(" << __LINE__ << "): " << STR(expr) << "failed\n";\
		logger::LogInfo(s.str());\
		_CrtDbgBreak(); \
	} \
} while (0)
#else
#define D3D_TRY(expr) expr
#endif
