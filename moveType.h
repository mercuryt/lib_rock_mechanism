#pragma once 

#include <string>
#include <list>

struct MoveType
{
	const std::string name;

	MoveType(std::string& n) : name(n) {}
};

static std::list<MoveType> moveTypes;

const MoveType* registerMoveType(std::string name)
{
	moveTypes.emplace_back(name);
	return &moveTypes.back();
}
