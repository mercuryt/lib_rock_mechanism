#pragma once
#include "../geometry/cuboidSet.h"
#include "../numericTypes/idTypes.h"
#include "../json.h"
class Area;
struct FluidGroup
{
	CuboidSet m_occupied;
	CuboidSet m_newlyAdded;
	CuboidSet m_noLongerOccupied;
	int64_t m_volume;
	FluidTypeId m_fluidType;
	FluidGroupId m_id;
	Distance m_lowZ;
	Distance m_highZ;
	bool m_merged = false;
	bool m_flowingUp = false;
	bool m_stable = false;
	bool m_aboveGround = false;
	bool m_drainingFromEdge = false;
	// If draining off edge an amount that ballences with the supply coming from FluidSource(s) then set m_steadyState to true.
	// This will modify flow rules to allow a river on a slope to expand into channels dug into its banks above it's leading zLevel but not overflow them at the zLevel transitions in the riverbed.
	//bool m_steadyState = false;
	// Used only on the first step after a group is created.
	bool m_checkMergeAll = true;
	FluidGroup(const CuboidSet& occupied, int64_t volume, FluidTypeId type, FluidGroupId id);
	void maybeDisplaceFromMoreDenseFluid(Area& area);
	void maybeDrainFromEdge(Area& area);
	void maybeExpand(Area& area);
	//void maybeExpandSteadyState(Area& area);
	void maybeConsolidate();
	void maybeSetLowerDensityAdjacentUnstable(Area& area);
	[[nodiscard]] std::vector<std::pair<CuboidSet, int64_t>> maybeSplit();
	void maybeMerge(Area& area);
	void updateHighAndLowZ();
	void updateHighAndLowZ(const CuboidSet& added);
	void mergeInto(Area& area, FluidGroup& other);
	void addFluid(int64_t quantity);
	void removeFluid(Area& area, int64_t quantity);
	[[nodiscard]] bool maybeDisplaceFromSolid(Area& area, const CuboidSet& solid);
	[[nodiscard]] int64_t trailingLevelFluidVolume() const;
	[[nodiscard]] int64_t trailingLevelFluidVolumePerPoint() const;
	[[nodiscard]] int64_t countPointsOnSurface(Area& area) const;
	[[nodiscard]] int64_t getVolume(const CuboidSet& cuboids) const;
	void removeFullFrom(CuboidSet& cuboids) const;
	[[nodiscard]] CuboidSet intersectionWithFull(const CuboidSet& cuboids) const;
	[[nodiscard]] bool isOverFull() const;
};