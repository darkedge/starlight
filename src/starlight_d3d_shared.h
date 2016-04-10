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
#define D3D_TRY(_expr) \
do { \
	HRESULT	_hr = _expr; \
	if (FAILED( _hr )) { \
		std::stringstream _s;\
		_s << __FILE__ << "(" << __LINE__ << "): " << STR(_expr) << "failed\n";\
		funcs->LogInfo(_s.str());\
	} \
} while (0)
#else
#define D3D_TRY(expr) expr
#endif
