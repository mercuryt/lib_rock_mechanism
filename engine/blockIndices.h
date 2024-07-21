#pragma once
#include "types.h"
#include <cassert>
#include <vector>
#include <ranges>
class BlockIndices
{
	std::vector<BlockIndex> data;
public:
	[[nodiscard]] std::vector<BlockIndex>::iterator find(BlockIndex block) { return std::ranges::find(data, block); }
	[[nodiscard]] std::vector<BlockIndex>::const_iterator find(BlockIndex block) const { return std::ranges::find(data, block); }
	[[nodiscard]] bool contains(BlockIndex block) const { return find(block) != data.end(); } 
	void add(BlockIndex block) { assert(!contains(block)); data.push_back(block); }
	void remove(BlockIndex block) { assert(contains(block)); auto found = find(block); (*found) = data.back(); data.resize(data.size() - 1); }
};
