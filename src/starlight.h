#pragma once
#include <stdint.h>

#define DEG2RAD 0.0174532925199433f
#define RAD2DEG 57.2957795130824f

struct float2 {
	float x;
	float y;

	inline float2 operator-(float2 vec) {
		return float2 {
			(x - vec.x),
			(y - vec.y)};
	}

	inline float2& operator-=(float2 vec) {
		*this = *this - vec;
		return *this;
	}

	inline float2 operator+(float2 vec) {
		return float2{
			(x + vec.x),
			(y + vec.y) };
	}

	inline float2& operator+=(float2 vec) {
		*this = *this + vec;
		return *this;
	}
};

struct int2 {
	int x;
	int y;
};

struct byte3 {
	uint8_t x;
	uint8_t y;
	uint8_t z;
};

#ifdef _MSC_VER
  #if _MSC_VER >= 1800
    #define SL_CALL __vectorcall
  #else
    #define SL_CALL __cdecl
  #endif
#else
#define SL_CALL
#endif

#define COUNT_OF(X) (sizeof(X) / sizeof((X)[0]))
#define ZERO_MEM(X, Y) (memset(X, 0, Y));

// For this to work, at least one .cpp file using this macro
// needs to be compiled on _every_ build, otherwise it is outdated.
#define SL_BUILD_DATE __DATE__ " " __TIME__
