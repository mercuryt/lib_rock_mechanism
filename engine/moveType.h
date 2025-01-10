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
	DataVector<std::wstring, MoveTypeId> m_name;
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
