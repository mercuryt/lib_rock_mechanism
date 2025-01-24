#pragma once

#include "types.h"
#include "dataVector.h"

#include <string>

struct MoveTypeParamaters
{
	std::wstring name;
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
	StrongVector<std::wstring, MoveTypeId> m_name;
	StrongBitSet<MoveTypeId> m_walk;
	StrongVector<uint8_t, MoveTypeId> m_climb;
	StrongBitSet<MoveTypeId> m_jumpDown;
	StrongBitSet<MoveTypeId> m_fly;
	StrongBitSet<MoveTypeId> m_breathless;
	StrongBitSet<MoveTypeId> m_onlyBreathsFluids;
	StrongVector<FluidTypeMap<CollisionVolume>, MoveTypeId> m_swim;
	StrongVector<FluidTypeSet, MoveTypeId> m_breathableFluids;
public:
	static void create(MoveTypeParamaters& paramaters);
	[[nodiscard]] static MoveTypeId byName(std::wstring name);
	[[nodiscard]] static std::wstring getName(MoveTypeId id);
	[[nodiscard]] static bool getWalk(MoveTypeId id);
	[[nodiscard]] static uint8_t getClimb(MoveTypeId id);
	[[nodiscard]] static bool getJumpDown(MoveTypeId id);
	[[nodiscard]] static bool getFly(MoveTypeId id);
	[[nodiscard]] static bool getBreathless(MoveTypeId id);
	[[nodiscard]] static bool getOnlyBreathsFluids(MoveTypeId id);
	[[nodiscard]] static FluidTypeMap<CollisionVolume>& getSwim(MoveTypeId id);
	[[nodiscard]] static FluidTypeSet& getBreathableFluids(MoveTypeId id);
};
inline MoveType moveTypeData;
