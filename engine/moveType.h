#pragma once 

#include "types.h"
#include "dataVector.h"

#include <string>

struct MoveTypeParamaters
{
	std::string name;
	bool walk;
	uint8_t climb;
	bool jumpDown;
	bool fly;
	bool breathless;
	bool onlyBreathsFluids;
	FluidTypeMap<CollisionVolume> swim = {};
	FluidTypeSet breathableFluids = {};
};
class MoveType
{
	static MoveType data;
	DataVector<std::string, MoveTypeId> m_name;
	DataBitSet<MoveTypeId> m_walk;
	DataVector<uint8_t, MoveTypeId> m_climb;
	DataBitSet<MoveTypeId> m_jumpDown;
	DataBitSet<MoveTypeId> m_fly;
	DataBitSet<MoveTypeId> m_breathless;
	DataBitSet<MoveTypeId> m_onlyBreathsFluids;
	DataVector<FluidTypeMap<CollisionVolume>, MoveTypeId> m_swim;
	DataVector<FluidTypeSet, MoveTypeId> m_breathableFluids;
public:
	static void create(MoveTypeParamaters& paramaters);
	[[nodiscard]] static MoveTypeId byName(std::string name);
	[[nodiscard]] static std::string getName(MoveTypeId id) { return data.m_name.at(id); }
	[[nodiscard]] static bool getWalk(MoveTypeId id) { return data.m_walk.at(id); }
	[[nodiscard]] static uint8_t getClimb(MoveTypeId id) { return data.m_climb.at(id); }
	[[nodiscard]] static bool getJumpDown(MoveTypeId id) { return data.m_jumpDown.at(id); }
	[[nodiscard]] static bool getFly(MoveTypeId id) { return data.m_fly.at(id); }
	[[nodiscard]] static bool getBreathless(MoveTypeId id) { return data.m_breathless.at(id); }
	[[nodiscard]] static bool getOnlyBreathsFluids(MoveTypeId id) { return data.m_onlyBreathsFluids.at(id); }
	[[nodiscard]] static FluidTypeMap<CollisionVolume>& getSwim(MoveTypeId id) { return data.m_swim.at(id); }
	[[nodiscard]] static FluidTypeSet& getBreathableFluids(MoveTypeId id) { return data.m_breathableFluids.at(id); }
};
