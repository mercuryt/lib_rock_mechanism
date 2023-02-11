#pragma once 

#include <string>
#include <vector>

struct MoveType
{
	std::string name;

	MoveType(std::string& n) : name(n) {}
};

std::vector<MoveType> moveTypes;

MoveType* registerMoveType(std::string name)
{
	moveTypes.emplace_back(name);
	return &moveTypes.back();
}
