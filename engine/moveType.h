#pragma once 

#include "fluidType.h"

#include <string>
#include <list>
#include <unordered_map>
#include <unordered_set>

struct MoveType
{
	const std::string name;
	bool walk;
	uint32_t climb;
	bool jumpDown;
	bool fly;
	bool breathless;
	bool onlyBreathsFluids;
	std::unordered_map<const FluidType*, uint32_t> swim;
	std::unordered_set<const FluidType*> breathableFluids;
	// Infastructure.
	bool operator==(const MoveType& moveType) const { return this == &moveType; }
	static const MoveType& byName(std::string name);
};
inline std::vector<MoveType> moveTypeDataStore;
