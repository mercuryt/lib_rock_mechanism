#pragma once

#include "../numericTypes/types.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/smallSet.h"

#include <string>

struct MoveTypeParamaters
{
	std::string name;
	bool surface;
	bool stairs;
	int climb;
	bool jumpDown;
	bool fly;
	bool breathless;
	bool onlyBreathsFluids;
	std::pair<FluidTypeId, Distance> floating;
	SmallMap<FluidTypeId, CollisionVolume> swim = {};
	SmallSet<FluidTypeId> breathableFluids = {};
};
class MoveType
{
	StrongVector<std::string, MoveTypeId> m_name;
	StrongBitSet<MoveTypeId> m_surface;
	StrongBitSet<MoveTypeId> m_stairs;
	StrongVector<int, MoveTypeId> m_climb;
	StrongBitSet<MoveTypeId> m_jumpDown;
	StrongBitSet<MoveTypeId> m_fly;
	StrongBitSet<MoveTypeId> m_breathless;
	StrongBitSet<MoveTypeId> m_onlyBreathsFluids;
	// For use by items and boats floating in fluids.
	StrongVector<std::pair<FluidTypeId, Distance>, MoveTypeId> m_floating;
	StrongVector<SmallMap<FluidTypeId, CollisionVolume>, MoveTypeId> m_swim;
	StrongVector<SmallSet<FluidTypeId>, MoveTypeId> m_breathableFluids;
	// Save for mutating.
	StrongVector<MoveTypeParamaters, MoveTypeId> m_paramaters;
public:
	static void create(const MoveTypeParamaters& paramaters);
	[[nodiscard]] static MoveTypeId byName(const std::string& name);
	[[nodiscard]] static std::string getName(const MoveTypeId id);
	[[nodiscard]] static bool getSurface(const MoveTypeId id);
	[[nodiscard]] static bool getStairs(const MoveTypeId id);
	[[nodiscard]] static int getClimb(const MoveTypeId id);
	[[nodiscard]] static bool getJumpDown(const MoveTypeId id);
	[[nodiscard]] static bool getFly(const MoveTypeId id);
	[[nodiscard]] static bool getBreathless(const MoveTypeId id);
	[[nodiscard]] static bool getOnlyBreathsFluids(const MoveTypeId id);
	[[nodiscard]] static std::pair<FluidTypeId, Distance> getFloating(const MoveTypeId id);
	[[nodiscard]] static SmallMap<FluidTypeId, CollisionVolume>& getSwim(const MoveTypeId id);
	[[nodiscard]] static SmallSet<FluidTypeId>& getBreathableFluids(const MoveTypeId id);
	[[nodiscard]] static MoveTypeId getOrCreateForFloat(FluidTypeId fluidType, Distance depth);
};
inline MoveType g_moveTypeData;
