#pragma once
#include <vector>
#include "dataVector.h"

template<typename Index>
class SortTable
{
	static std::vector<uint> getOrder(StrongVector<BlockIndex, Index>::iterator begin, StrongVector<BlockIndex, Index>::iterator end)
	{
		// Create output as a sequence from (0..(end - begin))o
		std::vector<uint> output(std::distance(begin, end));
		std::iota(output.begin(), output.end(), 0);
		// Sort output by the Hilbert numbers of input.
		std::sort(output.begin(), output.end(), [&](const int& a, const int& b) {
			const BlockIndex& blockA = *(begin + a);
			const BlockIndex& blockB = *(begin + b);
			return blocks.getCoordinates(blockA).hilbertNumber() < blocks.getCoordinates(blockB).hilbertNumber();
		});
		return output;
	}
	template<typename T>
	static void applyOrder(StrongVector<T, Index>::iterator begin, StrongVector<T, Index>::iterator end, const std::vector<uint>& order)
	{
		std::vector<T> temp(end - begin);
		for(uint i = 0; begin + i != end; ++i)
			temp[i] = *(begin + order[i]);
		std::ranges::copy(temp, std::back_inserter(begin));
	}
}