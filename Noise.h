#pragma once
#include <cstdint>
#include <array>

namespace noise {
	// Store the state of the noise functions (RNG)
	struct NoiseState {

	};

	namespace perlin {
		struct State {
			std::array<uint8_t, 512> p;
		};

		void Initialize(State* state, uint32_t seed);
		// x, y, z are non-uniform floats
		float Noise(State* state, float x, float y = 0.0f, float z = 0.0f);
	}
}
