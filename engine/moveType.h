#pragma once 

#include "fluidType.h"

#include <string>
#include <list>
#include <unordered_map>

struct MoveType
{
	const std::string name;
	bool walk;
	uint32_t climb;
	bool jumpDown;
	bool fly;
	std::unordered_map<const FluidType*, uint32_t> swim;
	// Infastructure.
	bool operator==(const MoveType& moveType) const { return this == &moveType; }
	static const MoveType& byName(std::string name);
};
inline std::vector<MoveType> moveTypeDataStore;
