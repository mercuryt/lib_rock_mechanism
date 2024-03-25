/*
a* A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Should only be used as inheritance target by Block, not intended to ever be instanced.
 */
#pragma once

#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <array>
#include <stack>
#include <memory>
#include <vector>
#include <string>

#include "deserializationMemo.h"
#include "moveType.h"
#include "shape.h"
#include "visionCuboid.h"
#include "fluidGroup.h"
#include "plant.h"
#include "blockFeature.h"
#include "designations.h"
#include "stockpile.h"
#include "farmFields.h"
#include "temperature.h"
#include "fire.h"
#include "hasShape.h"
#include "types.h"

class Area;
class Item;
class Actor;
class BlockHasActors;

// Fluid type and volume pairs are sorted by density, low to high.
// This is useful for resolving overfill.
// TODO: Maybe a vector of pairs would be better performance?
struct SortByDensity final
{
	bool operator()(const FluidType* a, const FluidType* b) const { return a->density < b->density; }
};
class Block final
{
	// If this block is solid stone, solid dirt, etc. then store it here. Otherwise nullptr.
	const MaterialType* m_solid;
	bool m_constructed;
public:
	DistanceInBlocks m_x, m_y, m_z;
	// Store area as a pointer rather then a reference to keep block default constructable.
	// This is required inorder to create the m_blocks structure before initalizing it.
	// TODO: emplace_back?
	Area* m_area;
	// Store adjacent in an array, with index determined by relative position.
	// below = 0, < = 1, ^ = 2, > = 3, v = 4, above = 5.
	std::array<Block*, 6> m_adjacents;
	// Contains only those adjacents which aren't null.
	// Sorted by density, low to high.
	// TODO: Try replacing with a flatmap.
	// TODO: HasFluids.
	std::map<const FluidType*, std::pair<uint32_t, FluidGroup*>, SortByDensity> m_fluids;
	uint32_t m_totalFluidVolume;
	// For mist.
	const FluidType* m_mist;
	//TODO: remove mistSource?
	const FluidType* m_mistSource;
	DistanceInBlocks m_mistInverseDistanceFromSource;
	// Store the location bucket this block belongs to.
	std::unordered_set<Actor*>* m_locationBucket;
	// Store the visionCuboid this block belongs to.
	VisionCuboid* m_visionCuboid;
	std::unordered_map<const MaterialType* , Fire>* m_fires;
	bool m_exposedToSky;
	bool m_underground;
	bool m_isEdge;
	bool m_outdoors;
	bool m_visible;
	uint32_t m_seed;
	BlockHasShapes m_hasShapes;
	Reservable m_reservable;
	HasPlant m_hasPlant;
	HasBlockFeatures m_hasBlockFeatures;
	BlockHasActors m_hasActors;
	BlockHasItems m_hasItems;
	HasDesignations m_hasDesignations;
	BlockIsPartOfStockPiles m_isPartOfStockPiles;
	IsPartOfFarmField m_isPartOfFarmField;
	BlockHasTemperature m_blockHasTemperature;

	Block();
	Block(const Block&) = delete;
	Block(Block&&) = delete;
	Block& operator=(const Block&) = delete;
	void setup(Area& area, uint32_t ax, uint32_t ay, uint32_t az);
	void recordAdjacent();
	std::vector<Block*> getAdjacentWithEdgeAdjacent() const;
	std::vector<Block*> getAdjacentWithEdgeAndCornerAdjacent() const;
	std::vector<Block*> getEdgeAndCornerAdjacentOnly() const;
	std::vector<Block*> getEdgeAdjacentOnly() const;
	std::vector<Block*> getEdgeAdjacentOnSameZLevelOnly() const;
	std::vector<Block*> getAdjacentOnSameZLevelOnly() const;
	std::vector<Block*> getEdgeAdjacentOnlyOnNextZLevelDown() const;
	std::vector<Block*> getEdgeAndCornerAdjacentOnlyOnNextZLevelDown() const;
	std::vector<Block*> getEdgeAdjacentOnlyOnNextZLevelUp() const;
	DistanceInBlocks distance(Block& block) const;
	DistanceInBlocks taxiDistance(const Block& block) const;
	bool squareOfDistanceIsMoreThen(const Block& block, uint32_t distanceSquared) const;
	bool isAdjacentToAny(std::unordered_set<Block*>& blocks) const;
	bool isAdjacentTo(Block& block) const;
	bool isAdjacentToIncludingCornersAndEdges(Block& block) const;
	bool isAdjacentTo(HasShape& hasShape) const;
	void setNotSolid();
	void setSolid(const MaterialType& materialType, bool contructed = false);
	bool isSolid() const;
	const MaterialType& getSolidMaterial() const;
	bool isConstructed() const { return m_constructed; }
	bool canSeeIntoFromAlways(const Block& block) const;
	void moveContentsTo(Block& block);
	Mass getMass() const;
	// Get block at offset coordinates. Can return nullptr.
	Block* offset(int32_t ax, int32_t ay, int32_t az) const;
	std::array<int32_t, 3> relativeOffsetTo(const Block& block) const; 
	bool canSeeThrough() const;
	bool canSeeThroughFrom(const Block& block) const;
	Facing facingToSetWhenEnteringFrom(const Block& block) const;
	Facing facingToSetWhenEnteringFromIncludingDiagonal(const Block& block, Facing inital = 0) const;
	bool isSupport() const;
	bool hasLineOfSightTo(Block& block) const;

	void spawnMist(const FluidType& fluidType, uint32_t maxMistSpread = 0);
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	// TODO: Is this being used?
	bool shapeAndMoveTypeCanEnterEver(const Shape& shape, const MoveType& moveType) const;
	// Get the FluidGroup for this fluid type in this block.
	FluidGroup* getFluidGroup(const FluidType& fluidType) const;
	// Add fluid, handle falling / sinking, group membership, excessive quantity sent to fluid group.
	void addFluid(uint32_t volume, const FluidType& fluidType);
	void removeFluid(uint32_t volume, const FluidType& fluidType);
	void removeFluidSyncronus(uint32_t volume, const FluidType& fluidType);
	bool fluidCanEnterCurrently(const FluidType& fluidType) const;
	bool isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const;
	uint32_t volumeOfFluidTypeCanEnter(const FluidType& fluidType) const;
	uint32_t volumeOfFluidTypeContains(const FluidType& fluidType) const;
	const FluidType& getFluidTypeWithMostVolume() const;
	bool fluidCanEnterEver() const;
	bool fluidTypeCanEnterCurrently(const FluidType& fluidType) const { return volumeOfFluidTypeCanEnter(fluidType) != 0; }
	// Move less dense fluids to their group's excessVolume until Config::maxBlockVolume is achieved.
	void resolveFluidOverfull();
	Block* getBlockBelow() { return m_adjacents[0]; }
	const Block* getBlockBelow() const { return m_adjacents[0]; }
	Block* getBlockAbove() { return m_adjacents[5]; }
	const Block* getBlockAbove() const { return m_adjacents[5]; }
	Block* getBlockNorth() { return m_adjacents[1]; }
	const Block* getBlockNorth() const { return m_adjacents[1]; }
	Block* getBlockWest() { return m_adjacents[2]; }
	const Block* getBlockWest() const { return m_adjacents[2]; }
	Block* getBlockSouth() { return m_adjacents[3]; }
	const Block* getBlockSouth() const { return m_adjacents[3]; }
	Block* getBlockEast() { return m_adjacents[4]; }
	const Block* getBlockEast() const { return m_adjacents[4]; }
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(bool exposed);
	void setBelowExposedToSky();
	void setBelowNotExposedToSky();
	void setBelowVisible();

	std::vector<Block*> selectBetweenCorners(Block* otherBlock) const;

	bool operator==(const Block& block) const;
	//TODO: Use std::function instead of template.
	template <typename F>
	std::unordered_set<Block*> collectAdjacentsWithCondition(F&& condition)
	{
		std::unordered_set<Block*> output;
		std::stack<Block*> openList;
		openList.push(this);
		output.insert(this);
		while(!openList.empty())
		{
			Block* block = openList.top();
			openList.pop();
			for(Block* adjacent : block->m_adjacents)
				if(adjacent && condition(*adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	template <typename F>
	Block* getBlockInRangeWithCondition(DistanceInBlocks range, F&& condition)
	{
		std::stack<Block*> open;
		open.push(this);
		std::unordered_set<Block*> closed;
		while(!open.empty())
		{
			Block* block = open.top();
			if(condition(*block))
				return block;
			open.pop();
			for(Block* adjacent : block->m_adjacents)
				if(adjacent && taxiDistance(*adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
					
				
		}
		return nullptr;
	}
	std::unordered_set<Block*> collectAdjacentsInRange(uint32_t range);
	std::vector<Block*> collectAdjacentsInRangeVector(uint32_t range);
	void loadFromJson(Json data, DeserializationMemo& deserializationMemo, uint32_t x, uint32_t y, uint32_t z);
	Json toJson() const;
	Json positionToJson() const;
};
inline void to_json(Json& data, const Block* const& block){ data = block->positionToJson(); }
inline void to_json(Json& data, const Block& block){ data = block.positionToJson(); }
inline void to_json(Json& data, const std::unordered_set<Block*>& blocks)
{
	for(const Block* block : blocks)
		data.push_back(block);
}
