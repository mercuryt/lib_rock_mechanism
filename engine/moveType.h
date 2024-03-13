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
	inline static std::vector<MoveType> data; 
	static const MoveType& byName(std::string name) { 
		auto found = std::ranges::find(data, name, &MoveType::name);
		assert(found != data.end());
		return *found;
	}
};
