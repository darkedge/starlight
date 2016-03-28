// http://www.codersnotes.com/notes/maths-lib-2016/
#pragma once

// The basic set of headers we need.
// Header rule 101: Don't pull in more than you need to.
// In particular, don't include stdio.h, windows.h or STL in headers
// that get used everywhere. Your compilation speed will skyrocket.
#include <stdint.h>
#include <math.h>
#include <xmmintrin.h>

// If you want it inlined, inline it.
// If you don't want it inlined, don't inline it.
// There is no in-between. Make your mind up :)
#define VM_INLINE	__forceinline

// Some helpers.
#define M_PI        3.14159265358979323846f
#define DEG2RAD(_a)	((_a)*M_PI/180.0f)
#define RAD2DEG(_a)	((_a)*180.0f/M_PI)
#define INT_MIN     (-2147483647 - 1)
#define INT_MAX     2147483647
#define FLT_MAX     3.402823466e+38F

// Shuffle helpers.
// Examples: SHUFFLE3(v, 0,1,2) leaves the vector unchanged.
//           SHUFFLE3(v, 0,0,0) splats the X coord out.
#define SHUFFLE3(V, X,Y,Z) float3(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(Z,Z,Y,X)))

// A basic float3 class.
// We try to maintain HLSL syntax as much as we can.
struct float3
{
	// Constructors.
	VM_INLINE float3() {}
	VM_INLINE explicit float3(const float *p) { m = _mm_set_ps(p[2], p[2], p[1], p[0]); }
	VM_INLINE explicit float3(float x, float y, float z) { m = _mm_set_ps(z, z, y, x); }
	VM_INLINE explicit float3(__m128 v) { m = v; }

	// Member accessors.
	// In an ideal world, we'd just use a union and allow the caller to access .x directly.
	// Unfortunately __m128 requires us to write wrappers. This sucks, and breaks HLSL
	// compatibility, but what can you do.
	VM_INLINE float x() const { return _mm_cvtss_f32(m); }
	VM_INLINE float y() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1))); }
	VM_INLINE float z() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2))); }

	// Helpers for common swizzles.
	VM_INLINE float3 yzx() const { return SHUFFLE3(*this, 1, 2, 0); }
	VM_INLINE float3 zxy() const { return SHUFFLE3(*this, 2, 0, 1); }

	// Unaligned store.
	VM_INLINE void store(float *p) const { p[0] = x(); p[1] = y(); p[2] = z(); }

	// Accessors to write to the components directly.
	// Generally speaking you should prefer to construct a new vector, rather than
	// modifiying the components of an existing vector.
	void setX(float x)
	{
		m = _mm_move_ss(m, _mm_set_ss(x));
	}
	void setY(float y)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(y));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(t, m);
	}
	void setZ(float z)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(z));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(t, m);
	}

	// For things that really need array-style access.
	VM_INLINE float operator[] (std::size_t i) const { return m.m128_f32[i]; };
	VM_INLINE float& operator[] (std::size_t i) { return m.m128_f32[i]; };

	__m128 m;
};

// A small helper to easily load integer arguments without having
// to manually cast everything.
VM_INLINE float3 float3i(int x, int y, int z) { return float3((float)x, (float)y, (float)z); }

// Helpers for initializing static data.
#define VCONST extern const __declspec(selectany)
struct vconstu
{
	union { uint32_t u[4]; __m128 v; };
	inline operator __m128() const { return v; }
};

VCONST vconstu vsignbits = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };

// Comparison operators need to return a SIMD bool.
// We'll just keep it simple here, but if you want you can actually implement
// this as a separate type with it's own operations.
typedef float3 bool3;

// Basic binary operators.
// Notice that these are all inline. It's not useful to us to ever actually
// step into these in the debugger, even in debug builds.
VM_INLINE float3 operator+ (float3 a, float3 b) { a.m = _mm_add_ps(a.m, b.m); return a; }
VM_INLINE float3 operator- (float3 a, float3 b) { a.m = _mm_sub_ps(a.m, b.m); return a; }
VM_INLINE float3 operator* (float3 a, float3 b) { a.m = _mm_mul_ps(a.m, b.m); return a; }
VM_INLINE float3 operator/ (float3 a, float3 b) { a.m = _mm_div_ps(a.m, b.m); return a; }
VM_INLINE float3 operator* (float3 a, float b) { a.m = _mm_mul_ps(a.m, _mm_set1_ps(b)); return a; }
VM_INLINE float3 operator/ (float3 a, float b) { a.m = _mm_div_ps(a.m, _mm_set1_ps(b)); return a; }
VM_INLINE float3 operator* (float a, float3 b) { b.m = _mm_mul_ps(_mm_set1_ps(a), b.m); return b; }
VM_INLINE float3 operator/ (float a, float3 b) { b.m = _mm_div_ps(_mm_set1_ps(a), b.m); return b; }
VM_INLINE float3& operator+= (float3 &a, float3 b) { a = a + b; return a; }
VM_INLINE float3& operator-= (float3 &a, float3 b) { a = a - b; return a; }
VM_INLINE float3& operator*= (float3 &a, float3 b) { a = a * b; return a; }
VM_INLINE float3& operator/= (float3 &a, float3 b) { a = a / b; return a; }
VM_INLINE float3& operator*= (float3 &a, float b) { a = a * b; return a; }
VM_INLINE float3& operator/= (float3 &a, float b) { a = a / b; return a; }
VM_INLINE bool3 operator==(float3 a, float3 b) { a.m = _mm_cmpeq_ps(a.m, b.m); return a; }
VM_INLINE bool3 operator!=(float3 a, float3 b) { a.m = _mm_cmpneq_ps(a.m, b.m); return a; }
VM_INLINE bool3 operator< (float3 a, float3 b) { a.m = _mm_cmplt_ps(a.m, b.m); return a; }
VM_INLINE bool3 operator> (float3 a, float3 b) { a.m = _mm_cmpgt_ps(a.m, b.m); return a; }
VM_INLINE bool3 operator<=(float3 a, float3 b) { a.m = _mm_cmple_ps(a.m, b.m); return a; }
VM_INLINE bool3 operator>=(float3 a, float3 b) { a.m = _mm_cmpge_ps(a.m, b.m); return a; }
VM_INLINE float3 min(float3 a, float3 b) { a.m = _mm_min_ps(a.m, b.m); return a; }
VM_INLINE float3 max(float3 a, float3 b) { a.m = _mm_max_ps(a.m, b.m); return a; }

// Unary operators.
VM_INLINE float3 operator- (float3 a) { return float3(_mm_setzero_ps()) - a; }
VM_INLINE float3 abs(float3 v) { v.m = _mm_andnot_ps(vsignbits, v.m); return v; }

// Horizontal min/max.
VM_INLINE float hmin(float3 v) {
	v = min(v, SHUFFLE3(v, 1, 0, 2));
	return min(v, SHUFFLE3(v, 2, 0, 1)).x();
}

VM_INLINE float hmax(float3 v) {
	v = max(v, SHUFFLE3(v, 1, 0, 2));
	return max(v, SHUFFLE3(v, 2, 0, 1)).x();
}

// 3D cross product.
VM_INLINE float3 cross(float3 a, float3 b) {
	// x  <-  a.y*b.z - a.z*b.y
	// y  <-  a.z*b.x - a.x*b.z
	// z  <-  a.x*b.y - a.y*b.x
	// We can save a shuffle by grouping it in this wacky order:
	return (a.zxy()*b - a*b.zxy()).zxy();
}

// Returns a 3-bit code where bit0..bit2 is X..Z
VM_INLINE unsigned mask(float3 v) { return _mm_movemask_ps(v.m) & 7; }

// Once we have a comparison, we can branch based on its results:
VM_INLINE bool any(bool3 v) { return mask(v) != 0; }
VM_INLINE bool all(bool3 v) { return mask(v) == 7; }

// Let's fill out the rest of the HLSL standard libary:
// (there's a few more but this will do to get you started)
VM_INLINE float3 clamp(float3 t, float3 a, float3 b) { return min(max(t, a), b); }
VM_INLINE float sum(float3 v) { return v.x() + v.y() + v.z(); }
VM_INLINE float dot(float3 a, float3 b) { return sum(a*b); }
VM_INLINE float length(float3 v) { return sqrtf(dot(v, v)); }
VM_INLINE float lengthSq(float3 v) { return dot(v, v); }
VM_INLINE float3 normalize(float3 v) { return v * (1.0f / length(v)); }
VM_INLINE float3 lerp(float3 a, float3 b, float t) { return a + (b - a)*t; }
