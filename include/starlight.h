#pragma once
#include <stdint.h>

#define DEG2RAD 0.0174532925199433f
#define RAD2DEG 57.2957795130824f

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// sse_vectormath does not have explicit 8-byte and 12-byte vectors
struct float3 {
	float x;
	float y;
	float z;

	inline float3 operator-(float3 vec) {
		return float3{
			(x - vec.x),
			(y - vec.y),
			(z - vec.z) };
	}

	inline float3& operator-=(float3 vec) {
		*this = *this - vec;
		return *this;
	}

	inline float3 operator+(float3 vec) {
		return float3{
			(x + vec.x),
			(y + vec.y),
			(z + vec.z) };
	}

	inline float3& operator+=(float3 vec) {
		*this = *this + vec;
		return *this;
	}
};

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

inline float2 operator*(float2 vec, float f) {
	return float2{
		(vec.x * f),
		(vec.y * f) };
}

inline float2 operator*(float f, float2 vec) {
	return float2{
		(f * vec.x),
		(f * vec.y) };
}

struct int2 {
	int x;
	int z;
};

inline int2 operator-(int2 a, int2 b) {
	return int2{
		a.x - b.x,
		a.z - b.z,
	};
}

inline bool operator==(int2 a, int2 b) {
	return a.x == b.x && a.z == b.z;
}

inline bool operator!=(int2 a, int2 b) {
	return !(a == b);
}

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
