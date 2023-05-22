#pragma once 

#include <string>
#include <list>
#include <unordered_map>


template<class FluidType>
struct BaseMoveType
{
	const std::string name;
	bool walk;
	std::unordered_map<const FluidType*, uint32_t> swim;
	uint32_t climb;
	bool jumpDown;
	bool fly;
};
