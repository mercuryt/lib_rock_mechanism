/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Should only be used as inheritance target by DerivedBlock, not intended to ever be instanced.
 */
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <array>
#include <stack>
#include <memory>
#include <vector>
#include <string>

#include "materialType.h"
#include "moveType.h"
#include "shape.h"
#include "fluidType.h"
#include "visionCuboid.h"
#include "fluidGroup.h"

class HasShape;
// Fluid type and volume pairs are sorted by density, low to high.
// This is useful for resolving overfill.
// TODO: Maybe a vector of pairs would be better performance?
struct SortByDensity
{
	bool operator()(const FluidType* a, const FluidType* b) const { return a->density < b->density; }
};
template<class DerivedBlock, class DerivedActor, class DerivedArea>
class BaseBlock
{
	// If this block is solid stone, solid dirt, etc. then store it here. Otherwise nullptr.
	const MaterialType* m_solid;
public:
	uint32_t m_x, m_y, m_z;
	// Store area as a pointer rather then a reference to keep block default constructable.
	// This is required inorder to create the m_blocks structure before initalizing it.
	// TODO: emplace_back?
	DerivedArea* m_area;
	// Store adjacent in an array, with index determined by relative position.
	// below = 0, < = 1, ^ = 2, > = 3, v = 4, above = 5.
	std::array<DerivedBlock*, 6> m_adjacents;
	std::vector<DerivedBlock*> m_adjacentsVector;
	// Store routes to other blocks by Shape*, MoveType* and destination*  
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::unordered_map<DerivedBlock*, 
		std::shared_ptr<std::vector<DerivedBlock*>>
	>>> m_routeCache;
	// Cache versions, check if matches the m_cacheVersion in area to see if cache is still valid.
	uint32_t m_routeCacheVersion;
	// Cache adjacent move costs. No version number: cleared only by changes to self / adjacent / diagonal. 
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<DerivedBlock*, uint32_t>>>> m_moveCostsCache;
	// Store a total occupied volume from actors.
	uint32_t m_totalDynamicVolume;
	// Store a total occupied volume from genericSolids and nongenerics.
	uint32_t m_totalStaticVolume;
	// Store a total occupied volume from fluids.
	uint32_t m_totalFluidVolume;
	// For loose generics: store material type and volume.
	std::unordered_map<const MaterialType*, uint32_t> m_genericSolids;
	// For fluids: store fluidType, volume, and FluidGroup pointer.
	// Sorted by density, low to high.
	// TODO: Try replacing with a flatmap.
	std::map<const FluidType*, std::pair<uint32_t, FluidGroup<DerivedBlock>*>, SortByDensity> m_fluids;
	// For mist.
	const FluidType* m_mist;
	const FluidType* m_mistSource;
	uint32_t m_mistInverseDistanceFromSource;
	// For immobile non generics could be items or buildings.
	std::unordered_map<HasShape*, uint32_t> m_nongenerics;
	// Track Actors and their volume which is in this block.
	std::unordered_map<DerivedActor*, uint32_t> m_actors;
	// Store the location bucket this block belongs to.
	std::unordered_set<DerivedActor*>* m_locationBucket;
	// Store the visionCuboid this block belongs to.
	VisionCuboid<DerivedBlock, DerivedActor, DerivedArea>* m_visionCuboid;

	// Constructor initalizes some members.
	BaseBlock();
	void setup(DerivedArea* a, uint32_t ax, uint32_t ay, uint32_t az);
	void recordAdjacent();
	std::vector<DerivedBlock*> getAdjacentWithEdgeAdjacent() const;
	std::vector<DerivedBlock*> getAdjacentWithEdgeAndCornerAdjacent() const;
	std::vector<DerivedBlock*> getEdgeAndCornerAdjacentOnly() const;
	std::vector<DerivedBlock*> getEdgeAdjacentOnly() const;
	std::vector<DerivedBlock*> getEdgeAdjacentOnSameZLevelOnly() const;
	std::vector<DerivedBlock*> getAdjacentOnSameZLevelOnly() const;
	std::vector<DerivedBlock*> getEdgeAdjacentOnlyOnNextZLevelDown() const;
	std::vector<DerivedBlock*> getEdgeAdjacentOnlyOnNextZLevelUp() const;
	uint32_t distance(DerivedBlock& block) const;
	uint32_t taxiDistance(DerivedBlock& block) const;
	bool isAdjacentToAny(std::unordered_set<DerivedBlock*>& blocks) const;
	void setNotSolid();
	void setSolid(const MaterialType* materialType);
	bool isSolid() const;
	const MaterialType* getSolidMaterial() const;
	void spawnMist(const FluidType* fluidType, uint32_t maxMistSpread = 0);
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	bool shapeAndMoveTypeCanEnterEver(const Shape* shape, const MoveType* moveType) const;
	// Get the FluidGroup for this fluid type in this block.
	FluidGroup<DerivedBlock>* getFluidGroup(const FluidType* fluidType) const;
	// Get block at offset coordinates.
	DerivedBlock* offset(int32_t ax, int32_t ay, int32_t az) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void addFluid(uint32_t volume, const FluidType* fluidType);
	void removeFluid(uint32_t volume, const FluidType* fluidType);
	bool actorCanEnterCurrently(DerivedActor& actor) const;
	bool canEnterEver(DerivedActor& actor) const;
	std::vector<std::pair<DerivedBlock*, uint32_t>> getMoveCosts(const Shape* shape, const MoveType* moveType);
	bool fluidCanEnterCurrently(const FluidType* fluidType) const;
	bool isAdjacentToFluidGroup(const FluidGroup<DerivedBlock>* fluidGroup) const;
	uint32_t volumeOfFluidTypeCanEnter(const FluidType* fluidType) const;
	uint32_t volumeOfFluidTypeContains(const FluidType* fluidType) const;
	// Move less dense fluids to their group's excessVolume until s_maxBlockVolume is achieved.
	void resolveFluidOverfull();
	void enter(DerivedActor& actor);
	void exit(DerivedActor& actor);
	// To be overriden by user code if diagonal movement allowed.
	void clearMoveCostsCacheForSelfAndAdjacent();
	std::vector<DerivedBlock*> selectBetweenCorners(DerivedBlock* otherBlock) const;

	bool operator==(const DerivedBlock& block) const;

	// User provided.
	bool moveTypeCanEnter(const MoveType* moveType) const;
	bool canEnterCurrently(DerivedActor* actor) const;
	bool anyoneCanEnterEver() const;
	uint32_t moveCost(const MoveType* moveType, DerivedBlock* origin) const;

	bool canSeeThroughFrom(const DerivedBlock& block) const;
	bool canSeeIntoFromAlways(DerivedBlock* block) const;
	float visionDistanceModifier() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(const FluidType* fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;

	void moveContentsTo(DerivedBlock* block);
};
