#pragma once
#include <array>
#include <glm/vec2.hpp>

template<uint64_t array_size>
struct ParticleArray {
	std::array<glm::vec2, array_size> positions;
	std::array<glm::vec2, array_size> velocities;

	static constexpr uint64_t size() {return array_size;}
};