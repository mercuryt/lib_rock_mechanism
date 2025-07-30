#pragma once

#include "numericTypes/types.h"
#include "dataStructures/strongVector.h"
#include "dataStructures/smallMap.h"
#include "dataStructures/smallSet.h"

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
	bool floating;
	SmallMap<FluidTypeId, CollisionVolume> swim = {};
	SmallSet<FluidTypeId> breathableFluids = {};
};
class MoveType
{
	StrongVector<std::string, MoveTypeId> m_name;
	StrongBitSet<MoveTypeId> m_walk;
	StrongVector<uint8_t, MoveTypeId> m_climb;
	StrongBitSet<MoveTypeId> m_jumpDown;
	StrongBitSet<MoveTypeId> m_fly;
	StrongBitSet<MoveTypeId> m_breathless;
	StrongBitSet<MoveTypeId> m_onlyBreathsFluids;
	// For use by items and boats floating in fluids.
	StrongBitSet<MoveTypeId> m_floating;
	StrongVector<SmallMap<FluidTypeId, CollisionVolume>, MoveTypeId> m_swim;
	StrongVector<SmallSet<FluidTypeId>, MoveTypeId> m_breathableFluids;
	// Save for mutating.
	StrongVector<MoveTypeParamaters, MoveTypeId> m_paramaters;
public:
	static void create(const MoveTypeParamaters& paramaters);
	[[nodiscard]] static MoveTypeId byName(const std::string& name);
	[[nodiscard]] static std::string getName(const MoveTypeId& id);
	[[nodiscard]] static bool getWalk(const MoveTypeId& id);
	[[nodiscard]] static uint8_t getClimb(const MoveTypeId& id);
	[[nodiscard]] static bool getJumpDown(const MoveTypeId& id);
	[[nodiscard]] static bool getFly(const MoveTypeId& id);
	[[nodiscard]] static bool getBreathless(const MoveTypeId& id);
	[[nodiscard]] static bool getOnlyBreathsFluids(const MoveTypeId& id);
	[[nodiscard]] static bool getFloating(const MoveTypeId& id);
	[[nodiscard]] static SmallMap<FluidTypeId, CollisionVolume>& getSwim(const MoveTypeId& id);
	[[nodiscard]] static SmallSet<FluidTypeId>& getBreathableFluids(const MoveTypeId& id);
};
inline MoveType moveTypeData;
