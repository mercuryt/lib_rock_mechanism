#pragma once 

#include "fluidType.h"
#include "types.h"

#include <string>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <utility>

struct MoveTypeParamaters
{
	std::string name;
	bool walk;
	uint32_t climb;
	bool jumpDown;
	bool fly;
	bool breathless;
	bool onlyBreathsFluids;
	std::unordered_map<const FluidType*, CollisionVolume> swim;
	std::unordered_set<const FluidType*> breathableFluids;
};
class MoveType
{
	static MoveType data;
	DataVector<std::string, MoveTypeId> m_name;
	DataVector<bool, MoveTypeId> m_walk;
	DataVector<uint32_t, MoveTypeId> m_climb;
	DataVector<bool, MoveTypeId> m_jumpDown;
	DataVector<bool, MoveTypeId> m_fly;
	DataVector<bool, MoveTypeId> m_breathless;
	DataVector<bool, MoveTypeId> m_onlyBreathsFluids;
	DataVector<std::unordered_map<const FluidType*, CollisionVolume>, MoveTypeId> m_swim;
	DataVector<std::unordered_set<const FluidType*>, MoveTypeId> m_breathableFluids;
public:
	static MoveTypeId byName(std::string name);
	void create(MoveTypeParamaters& paramaters);
	[[nodiscard]] static std::string getname(MoveTypeId id) { return data.m_name.at(id); }
	[[nodiscard]] static bool getwalk(MoveTypeId id) { return data.m_walk.at(id); }
	[[nodiscard]] static uint32_t getclimb(MoveTypeId id) { return data.m_climb.at(id); }
	[[nodiscard]] static bool getjumpDown(MoveTypeId id) { return data.m_jumpDown.at(id); }
	[[nodiscard]] static bool getfly(MoveTypeId id) { return data.m_fly.at(id); }
	[[nodiscard]] static bool getbreathless(MoveTypeId id) { return data.m_breathless.at(id); }
	[[nodiscard]] static bool getonlyBreathsFluids(MoveTypeId id) { return data.m_onlyBreathsFluids.at(id); }
	[[nodiscard]] static std::unordered_map<const FluidType*, CollisionVolume> getswim(MoveTypeId id) { return data.m_swim.at(id); }
	[[nodiscard]] static std::unordered_set<const FluidType*> getbreathableFluids(MoveTypeId id) { return data.m_breathableFluids.at(id); }
};
