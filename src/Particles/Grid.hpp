#pragma once
#include <array>
#include <glm/vec2.hpp>

struct CellIndex {
	int16_t x;
	int16_t y;
};

template<unsigned int MaxMembers>
struct GridCell {
	std::array<uint32_t, MaxMembers> members;
};

// Supports NumCells up to 255
template<int16_t NumCells>
struct Grid {
	std::array<GridCell<32>, NumCells*NumCells> cells;
	std::array<uint8_t, NumCells*NumCells> cell_sizes;

	static constexpr int16_t get_num_cells() { return NumCells; }
	static constexpr float get_cell_size() { return 2.f / NumCells; }

	static constexpr CellIndex clamp_cell_index(const CellIndex id) {
		return { glm::clamp(id.x, 0, NumCells), glm::clamp(id.y, 0, NumCells) };
	}

	static constexpr CellIndex get_cell_index(const glm::vec2 &position) {
		return {
			static_cast<int16_t>(NumCells * position.x),
			static_cast<int16_t>(NumCells * position.y)
		};
	}

	static constexpr unsigned int get_cell(const glm::vec2 &position) {
		return get_cell(get_cell_index(position));
	}

	static constexpr unsigned int get_cell(const CellIndex id) {
		return static_cast<uint16_t>(glm::clamp<int16_t>(static_cast<int16_t>(id.x), 0, NumCells)) + static_cast<uint16_t>(glm::clamp<int16_t>(static_cast<int16_t>(id.y), 0, NumCells)) * NumCells;
	}

	static constexpr glm::vec2 get_cell_pos(const unsigned int cell) {
		return {
			(((cell % NumCells)/static_cast<float>(NumCells)) * 2.f - 1.f),
			(((cell / NumCells)/static_cast<float>(NumCells)) * 2.f - 1.f),
		};
	}

	void reset() {
		for (auto &size : cell_sizes) {
			size = 0;
		}
	}

	void add_element(const uint16_t member, const glm::vec2 &position) {
		const uint16_t cell_id = get_cell(position);
		if (cell_id < NumCells*NumCells) {
			auto &cell = cells[cell_id];
			uint8_t &cell_size = cell_sizes[cell_id];
			if (cell.members.size() != cell_size) {
				cell.members[cell_size] = member;
				++cell_size;
			}
		}
	}
};
