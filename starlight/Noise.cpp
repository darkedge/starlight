#include "Noise.h"
#include <cmath>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <random>

// Perlin noise

float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
float Lerp(float t, float a, float b) { return a + t * (b - a); }

float Grad(uint8_t hash, float x, float y, float z) {
	uint8_t h = hash & 15;
	float u = (h < 8) ? x : y;
	float v = (h < 4) ? y : (h == 12 || h == 14) ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

void noise::perlin::Initialize(State* state, uint32_t seed)
{
	assert(state);
	auto &p = state->p;
	std::iota(p.begin(), p.begin() + 256, 0);
	std::default_random_engine engine(seed);
	std::shuffle(p.begin(), p.begin() + 256, engine);
	std::copy(p.begin(), p.begin() + 256, p.begin() + 256);
}

float noise::perlin::Noise(perlin::State* state, float x, float y, float z) {
	assert(state);

	int32_t X = (int32_t) std::floor(x) & 0xFF;
	int32_t Y = (int32_t) std::floor(y) & 0xFF;
	int32_t Z = (int32_t) std::floor(z) & 0xFF;
	x -= std::floor(x);
	y -= std::floor(y);
	z -= std::floor(z);
	float u = Fade(x);
	float v = Fade(y);
	float w = Fade(z);
	int32_t A = state->p[X] + Y;
	int32_t AA = state->p[A] + Z;
	int32_t AB = state->p[A + 1] + Z;
	int32_t B = state->p[X + 1] + Y;
	int32_t BA = state->p[B] + Z;
	int32_t BB = state->p[B + 1] + Z;

	return Lerp(w, Lerp(v, Lerp(u, Grad(state->p[AA], x, y, z),
		Grad(state->p[BA], x - 1, y, z)),
		Lerp(u, Grad(state->p[AB], x, y - 1, z),
			Grad(state->p[BB], x - 1, y - 1, z))),
		Lerp(v, Lerp(u, Grad(state->p[AA + 1], x, y, z - 1),
			Grad(state->p[BA + 1], x - 1, y, z - 1)),
			Lerp(u, Grad(state->p[AB + 1], x, y - 1, z - 1),
				Grad(state->p[BB + 1], x - 1, y - 1, z - 1))));
}
