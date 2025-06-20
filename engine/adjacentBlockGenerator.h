/*
 * intented to be returned by various get adjacent blocks methods, is it more efficiente then array due to short circut or is it encomberance to paralelization? need to profile.
 */
#pragma once
#include "numericTypes/index.h"
#include "numericTypes/types.h"
#include "blocks/blocks.h"
#include <array>
#include <cstdint>

template<uint8_t Size>
class AdjacentBlockGenerator
{
	using Data = std::array<std::array<int8_t, 3>, Size>;
	Data::iterator m_iter;
	Data::iterator m_end;
public:
	AdjacentBlockGenerator(const Data& offsets) :
		m_iter(offsets.begin()), m_end(offsets.end()) { }
	[[nodiscard]] BlockIndex next(Point3D coordinates, Blocks& blocks, bool isOnEdge) const
	{
		BlockIndex output;
		const auto& offsets = *m_iter;
		if(isOnEdge)
			output = blocks.maybeGetIndexFromOffsetOnEdge(coordinates, offsets);
		else
			output = blocks.getIndex(coordinates.x + offsets[0], coordinates.y + offsets[1], coordinates.z + offsets[2]);
		++m_iter;
		return output;
	}
	[[nodiscard]] bool done() const { return m_iter == m_end; }
};
