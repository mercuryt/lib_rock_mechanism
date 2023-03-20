/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Should only be used as inheritance target by Block, not intended to ever be instanced.
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
class VisionRequest;
class FluidGroup;
class HasShape;
class Actor;
class Area;
class Block;

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
	Area* m_area;
	// Store adjacent in an array, with index determined by relative position.
	// below = 0, < = 1, ^ = 2, > = 3, v = 4, above = 5.
	std::array<Block*, 6> m_adjacents;
	std::vector<Block*> m_adjacentsVector;
	// Store routes to other blocks by Shape*, MoveType* and destination*  
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::unordered_map<Block*, 
		std::shared_ptr<std::vector<Block*>>
	>>> m_routeCache;
	// Cache versions, check if matches the m_cacheVersion in area to see if cache is still valid.
	uint32_t m_routeCacheVersion;
	// Cache adjacent move costs. No version number: cleared only by changes to self / adjacent / diagonal. 
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<Block*, uint32_t>>>> m_moveCostsCache;

	// Store a total occupied volume from actors.
	uint32_t m_totalDynamicVolume;
	// Store a total occupied volume from genericSolids and nongenerics.
	uint32_t m_totalStaticVolume;
	// Store a total occupied volume from fluids.
	uint32_t m_totalFluidVolume;
	// For loose generics: store material type and volume.
	std::unordered_map<const MaterialType*, uint32_t> m_genericSolids;
	// For fluids: store fluidType, volume, and FluidGroup pointer.
	std::map<const FluidType*, std::pair<uint32_t, FluidGroup*>, SortByDensity> m_fluids;
	// For immobile non generics could be items or buildings.
	std::unordered_map<HasShape*, uint32_t> m_nongenerics;
	// Track Actors and their volume which is in this block.
	std::unordered_map<Actor*, uint32_t> m_actors;
	// Store the location bucket this block belongs to.
	std::unordered_set<Actor*>* m_locationBucket;

	// Constructor initalizes some members.
	baseBlock();
	void setup(Area* a, uint32_t ax, uint32_t ay, uint32_t az);
	void recordAdjacent();
	std::vector<Block*> getAdjacentWithEdgeAdjacent() const;
	std::vector<Block*> getAdjacentWithEdgeAndCornerAdjacent() const;
	std::vector<Block*> getEdgeAndCornerAdjacentOnly() const;
	std::vector<Block*> getEdgeAdjacentOnly() const;
	uint32_t distance(Block* block) const;
	uint32_t taxiDistance(Block* block) const;
	bool isAdjacentToAny(std::unordered_set<Block*>& blocks) const;
	void setNotSolid();
	void setSolid(const MaterialType* materialType);
	bool isSolid() const;
	const MaterialType* getSolidMaterial() const;
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	bool shapeAndMoveTypeCanEnterEver(const Shape* shape, const MoveType* moveType) const;
	// Get the FluidGroup for this fluid type in this block.
	FluidGroup* getFluidGroup(const FluidType* fluidType) const;
	// Get block at offset coordinates.
	Block* offset(int32_t ax, int32_t ay, int32_t az) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void addFluid(uint32_t volume, const FluidType* fluidType);
	void removeFluid(uint32_t volume, const FluidType* fluidType);
	bool shapeCanEnterCurrently(const Shape* shape) const;
	bool canEnterEver(Actor* actor) const;
	std::vector<std::pair<Block*, uint32_t>> getMoveCosts(const Shape* shape, const MoveType* moveType);
	bool fluidCanEnterCurrently(const FluidType* fluidType) const;
	bool isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const;
	uint32_t volumeOfFluidTypeCanEnter(const FluidType* fluidType) const;
	uint32_t volumeOfFluidTypeContains(const FluidType* fluidType) const;
	// Move less dense fluids to their group's excessVolume until s_maxBlockVolume is achieved.
	void resolveFluidOverfull();
	void enter(Actor* actor);
	void exit(Actor* actor);
	// To be overriden by user code if diagonal movement allowed.
	void clearMoveCostsCacheForSelfAndAdjacent();
	std::string toS();

	// User provided.
	bool moveTypeCanEnter(const MoveType* moveType) const;
	bool canEnterCurrently(Actor* actor) const;
	bool canEnterEver() const;
	uint32_t moveCost(const MoveType* moveType, Block* origin) const;

	bool canSeeThroughFrom(Block* block) const;
	float visionDistanceModifier() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(const FluidType* fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;

	void moveContentsTo(Block* block);
};
