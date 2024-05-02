#pragma once
#include "../types.h"
#include "../config.h"
#include "mistDisperseEvent.h"
struct FluidType;
class FluidGroup;
class Block;
struct DeserializationMemo;
// Fluid type and volume pairs are sorted by density, low to high.
// This is useful for resolving overfill.
// TODO: Maybe a vector of pairs would be better performance?
struct SortByDensity final
{
	bool operator()(const FluidType* a, const FluidType* b) const;
};
class BlockHasFluids final
{
	Block& m_block;
	// Sorted by density, low to high.
	// TODO: Try replacing with a flatmap.
	// TODO: HasFluids.
	std::map<const FluidType*, std::pair<CollisionVolume, FluidGroup*>, SortByDensity> m_fluids;
	CollisionVolume m_totalFluidVolume = 0;
	// For mist.
	const FluidType* m_mist = nullptr;
	//TODO: remove mistSource?
	const FluidType* m_mistSource = nullptr;
	DistanceInBlocks m_mistInverseDistanceFromSource = 0;
public:
	BlockHasFluids(Block& block) : m_block(block) { }
	void load(const Json& data, const DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void spawnMist(const FluidType& fluidType, DistanceInBlocks maxMistSpread = 0);
	// Get the FluidGroup for this fluid type in this block.
	FluidGroup* getFluidGroup(const FluidType& fluidType) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void addFluid(CollisionVolume volume, const FluidType& fluidType);
	void removeFluid(CollisionVolume volume, const FluidType& fluidType);
	void removeFluidSyncronus(CollisionVolume volume, const FluidType& fluidType);
	// Move less dense fluids to their group's excessVolume until Config::maxBlockVolume is achieved.
	void resolveFluidOverfull();
	void onBlockSetSolid();
	void onBlockSetNotSolid();
	[[nodiscard]] bool fluidCanEnterCurrently(const FluidType& fluidType) const;
	[[nodiscard]] bool isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const;
	[[nodiscard]] CollisionVolume volumeOfFluidTypeCanEnter(const FluidType& fluidType) const;
	[[nodiscard]] CollisionVolume volumeOfFluidTypeContains(const FluidType& fluidType) const;
	[[nodiscard]] const FluidType& getFluidTypeWithMostVolume() const;
	[[nodiscard]] bool fluidCanEnterEver() const;
	[[nodiscard]] bool fluidTypeCanEnterCurrently(const FluidType& fluidType) const { return volumeOfFluidTypeCanEnter(fluidType) != 0; }
	[[nodiscard]] bool any() const { return m_totalFluidVolume; }
	[[nodiscard]] bool contains(const FluidType& fluidType) const { return m_fluids.contains(&fluidType); }
	[[nodiscard]] std::map<const FluidType*, std::pair<CollisionVolume, FluidGroup*>, SortByDensity>& getAll() { return m_fluids; }
	friend class FluidGroup;
	friend class FillQueue;
	friend class DrainQueue;
	friend class MistDisperseEvent;
	// For testing.
	[[nodiscard]] CollisionVolume getTotalVolume() const { return m_totalFluidVolume; }
	[[nodiscard]] const FluidType* getMist() const { return m_mist; }
	void setMistSource(const FluidType* fluidType) { m_mistSource = fluidType; }
};
