#pragma once 

#include <string>
#include <list>
#include <unordered_map>
class FluidType;

struct MoveType
{
	const std::string name;
	bool walk;
	std::unordered_map<const FluidType*, uint32_t> swim;
	uint32_t climb;
	bool jumpDown;
	bool fly;

	MoveType(std::string& n, bool w, std::unordered_map<const FluidType*, uint32_t> s, u_int32_t c, bool jd, bool f) : 
		name(n), walk(w), swim(s), climb(c), jumpDown(jd), fly(f) {}
};

static std::list<MoveType> moveTypes;

inline const MoveType* registerMoveType(std::string name, bool walk, std::unordered_map<const FluidType*, uint32_t> swim, u_int32_t climb, bool jumpDown, bool fly)
{
	moveTypes.emplace_back(name, walk, swim, climb, jumpDown, fly);
	return &moveTypes.back();
}
