#pragma once

#include "index.h"
#include <array>
#include <cstdint>
template<uint8_t size>
class BlockIndexArrayNotNull
{
	using Data = std::array<BlockIndex, size>;
	Data data;
public:
	BlockIndexArrayNotNull(const Data& d) : data(d) { }
	BlockIndexArrayNotNull(const Data&& d) : data(d) { }
	BlockIndex& operator[](uint8_t index) { assert(data[index].exists()); return data[index]; }
	[[nodiscard]] auto begin() -> Data::iterator { return data.begin(); }
	[[nodiscard]] auto end() -> Data::iterator { return std::ranges::find(data, BlockIndex::null()); }
	[[nodiscard]] auto begin() const -> Data::const_iterator { return data.begin(); }
	[[nodiscard]] auto end() const -> Data::const_iterator { return std::ranges::find(data, BlockIndex::null()); }
	[[nodiscard]] auto toArray() const -> Data& { return data; }
};
