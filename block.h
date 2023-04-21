/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Should only be used as inheritance target by BLOCK, not intended to ever be instanced.
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

class RouteRequest;
class VisionCuboid;
class VisionRequest;
class FluidGroup;
class HasShape;
class ACTOR;
class AREA;
class BLOCK;

// Fluid type and volume pairs are sorted by density, low to high.
// This is useful for resolving overfill.
// TODO: Maybe a vector of pairs would be better performance?
struct SortByDensity
{
	bool operator()(const FluidType* a, const FluidType* b) const { return a->density < b->density; }
};
class baseBlock
{
	// If this block is solid stone, solid dirt, etc. then store it here. Otherwise nullptr.
	const MaterialType* m_solid;
public:
	uint32_t m_x, m_y, m_z;
	// Store area as a pointer rather then a reference to keep block default constructable.
	// This is required inorder to create the m_blocks structure before initalizing it.
	// TODO: emplace_back?
	AREA* m_area;
	// Store adjacent in an array, with index determined by relative position.
	// below = 0, < = 1, ^ = 2, > = 3, v = 4, above = 5.
	std::array<BLOCK*, 6> m_adjacents;
	std::vector<BLOCK*> m_adjacentsVector;
	// Store routes to other blocks by Shape*, MoveType* and destination*  
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::unordered_map<BLOCK*, 
		std::shared_ptr<std::vector<BLOCK*>>
	>>> m_routeCache;
	// Cache versions, check if matches the m_cacheVersion in area to see if cache is still valid.
	uint32_t m_routeCacheVersion;
	// Cache adjacent move costs. No version number: cleared only by changes to self / adjacent / diagonal. 
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<BLOCK*, uint32_t>>>> m_moveCostsCache;
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
	std::map<const FluidType*, std::pair<uint32_t, FluidGroup*>, SortByDensity> m_fluids;
	// For mist.
	const FluidType* m_mist;
	const FluidType* m_mistSource;
	uint32_t m_mistInverseDistanceFromSource;
	// For immobile non generics could be items or buildings.
	std::unordered_map<HasShape*, uint32_t> m_nongenerics;
	// Track Actors and their volume which is in this block.
	std::unordered_map<ACTOR*, uint32_t> m_actors;
	// Store the location bucket this block belongs to.
	std::unordered_set<ACTOR*>* m_locationBucket;
	// Store the visionCuboid this block belongs to.
	VisionCuboid* m_visionCuboid;

	// Constructor initalizes some members.
	baseBlock();
	void setup(AREA* a, uint32_t ax, uint32_t ay, uint32_t az);
	void recordAdjacent();
	std::vector<BLOCK*> getAdjacentWithEdgeAdjacent() const;
	std::vector<BLOCK*> getAdjacentWithEdgeAndCornerAdjacent() const;
	std::vector<BLOCK*> getEdgeAndCornerAdjacentOnly() const;
	std::vector<BLOCK*> getEdgeAdjacentOnly() const;
	std::vector<BLOCK*> getEdgeAdjacentOnSameZLevelOnly() const;
	std::vector<BLOCK*> getAdjacentOnSameZLevelOnly() const;
	std::vector<BLOCK*> getEdgeAdjacentOnlyOnNextZLevelDown() const;
	std::vector<BLOCK*> getEdgeAdjacentOnlyOnNextZLevelUp() const;
	uint32_t distance(BLOCK& block) const;
	uint32_t taxiDistance(BLOCK& block) const;
	bool isAdjacentToAny(std::unordered_set<BLOCK*>& blocks) const;
	void setNotSolid();
	void setSolid(const MaterialType* materialType);
	bool isSolid() const;
	const MaterialType* getSolidMaterial() const;
	void spawnMist(const FluidType* fluidType, uint32_t maxMistSpread = 0);
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	bool shapeAndMoveTypeCanEnterEver(const Shape* shape, const MoveType* moveType) const;
	// Get the FluidGroup for this fluid type in this block.
	FluidGroup* getFluidGroup(const FluidType* fluidType) const;
	// Get block at offset coordinates.
	BLOCK* offset(int32_t ax, int32_t ay, int32_t az) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void addFluid(uint32_t volume, const FluidType* fluidType);
	void removeFluid(uint32_t volume, const FluidType* fluidType);
	bool actorCanEnterCurrently(ACTOR& actor) const;
	bool canEnterEver(ACTOR& actor) const;
	std::vector<std::pair<BLOCK*, uint32_t>> getMoveCosts(const Shape* shape, const MoveType* moveType);
	bool fluidCanEnterCurrently(const FluidType* fluidType) const;
	bool isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const;
	uint32_t volumeOfFluidTypeCanEnter(const FluidType* fluidType) const;
	uint32_t volumeOfFluidTypeContains(const FluidType* fluidType) const;
	// Move less dense fluids to their group's excessVolume until s_maxBlockVolume is achieved.
	void resolveFluidOverfull();
	void enter(ACTOR& actor);
	void exit(ACTOR& actor);
	// To be overriden by user code if diagonal movement allowed.
	void clearMoveCostsCacheForSelfAndAdjacent();
	std::vector<BLOCK*> selectBetweenCorners(BLOCK* otherBlock) const;

	bool operator==(const BLOCK& block) const;

	// User provided.
	bool moveTypeCanEnter(const MoveType* moveType) const;
	bool canEnterCurrently(ACTOR* actor) const;
	bool anyoneCanEnterEver() const;
	uint32_t moveCost(const MoveType* moveType, BLOCK* origin) const;

	bool canSeeThroughFrom(const BLOCK& block) const;
	bool canSeeIntoFromAlways(BLOCK* block) const;
	float visionDistanceModifier() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(const FluidType* fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;

	void moveContentsTo(BLOCK* block);
};
static_assert(std::default_initializable<baseBlock>);
