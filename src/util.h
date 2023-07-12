#pragma once

#include <unordered_set>
#include <stack>
#include <vector>
#include <queue>
#include <cstdint>
#include <string>
#include <cassert>

class Block;

namespace util
{
	template <typename F>
	static std::unordered_set<Block*> collectAdjacentsWithCondition(F&& condition, Block& block);
	//static std::unordered_set<Block*> collectAdjacentsInRange(uint32_t range, Block& block);
	//static std::vector<Block*> collectAdjacentsInRangeVector(uint32_t range, Block& block);
	template<typename PathT, typename DestinationT>
	static Block* findWithPathCondition(PathT&& pathCondition, DestinationT&& destinationCondition, Block& block);
	// This was suposed to replace collectAdjacentsWithCondition for detecting splits in fluidGroups but it was slower
	template <typename F>
	static std::vector<std::unordered_set<Block*>> findGroups(F&& condition, std::unordered_set<Block*>& blocks);
	int scaleByPercent(int base, uint32_t percent);
	int scaleByInversePercent(int base, uint32_t percent);
	int scaleByPercentRange(int min, int max, uint32_t percent);
	int scaleByFraction(int base, uint32_t numerator, uint32_t denominator);
	int scaleByInverseFraction(int base, uint32_t numerator, uint32_t denominator);
	std::string wideStringToString(const std::wstring& wstr);
};
