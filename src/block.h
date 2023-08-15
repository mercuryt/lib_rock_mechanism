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

class Area;
class Item;
class Actor;

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
public:
	uint32_t m_x, m_y, m_z;
	// Store area as a pointer rather then a reference to keep block default constructable.
	// This is required inorder to create the m_blocks structure before initalizing it.
	// TODO: emplace_back?
	Area* m_area;
	// Store adjacent in an array, with index determined by relative position.
	// below = 0, < = 1, ^ = 2, > = 3, v = 4, above = 5.
	std::array<Block*, 6> m_adjacents;
	std::vector<Block*> m_adjacentsVector;
	// Sorted by density, low to high.
	// TODO: Try replacing with a flatmap.
	// TODO: HasFluids.
	std::map<const FluidType*, std::pair<uint32_t, FluidGroup*>, SortByDensity> m_fluids;
	uint32_t m_totalFluidVolume;
	// For mist.
	const FluidType* m_mist;
	const FluidType* m_mistSource;
	uint32_t m_mistInverseDistanceFromSource;
	// Store the location bucket this block belongs to.
	std::unordered_set<Actor*>* m_locationBucket;
	// Store the visionCuboid this block belongs to.
	VisionCuboid* m_visionCuboid;
	std::unique_ptr<Fire> m_fire;
	bool m_exposedToSky;
	bool m_underground;
	bool m_isEdge;
	bool m_outdoors;
	BlockHasShapes m_hasShapes;
	Reservable m_reservable;
	HasPlant m_hasPlant;
	HasBlockFeatures m_hasBlockFeatures;
	BlockHasActors m_hasActors;
	BlockHasItems m_hasItems;
	HasDesignations m_hasDesignations;
	BlockIsPartOfStockPile m_isPartOfStockPile;
	IsPartOfFarmField m_isPartOfFarmField;
	BlockHasTemperature m_blockHasTemperature;

	// Constructor initalizes some members.
	Block();
	void setup(Area& a, uint32_t ax, uint32_t ay, uint32_t az);
	void recordAdjacent();
	std::vector<Block*> getAdjacentWithEdgeAdjacent() const;
	std::vector<Block*> getAdjacentWithEdgeAndCornerAdjacent() const;
	std::vector<Block*> getEdgeAndCornerAdjacentOnly() const;
	std::vector<Block*> getEdgeAdjacentOnly() const;
	std::vector<Block*> getEdgeAdjacentOnSameZLevelOnly() const;
	std::vector<Block*> getAdjacentOnSameZLevelOnly() const;
	std::vector<Block*> getEdgeAdjacentOnlyOnNextZLevelDown() const;
	std::vector<Block*> getEdgeAdjacentOnlyOnNextZLevelUp() const;
	uint32_t distance(Block& block) const;
	uint32_t taxiDistance(Block& block) const;
	bool isAdjacentToAny(std::unordered_set<Block*>& blocks) const;
	bool isAdjacentTo(Block& block) const;
	void setNotSolid();
	void setSolid(const MaterialType& materialType);
	bool isSolid() const;
	const MaterialType& getSolidMaterial() const;
	bool canSeeIntoFromAlways(const Block& block) const;
	void moveContentsTo(Block& block);
	uint32_t getMass() const;
	// Get block at offset coordinates. Can return nullptr.
	Block* offset(int32_t ax, int32_t ay, int32_t az) const;
	bool canSeeThrough() const;
	bool canSeeThroughFrom(const Block& block) const;
	uint8_t facingToSetWhenEnteringFrom(const Block& block) const;
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
	bool fluidCanEnterCurrently(const FluidType& fluidType) const;
	bool isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const;
	uint32_t volumeOfFluidTypeCanEnter(const FluidType& fluidType) const;
	uint32_t volumeOfFluidTypeContains(const FluidType& fluidType) const;
	const FluidType& getFluidTypeWithMostVolume() const;
	bool fluidCanEnterEver() const;
	bool fluidTypeCanEnterCurrently(const FluidType& fluidType) const { return volumeOfFluidTypeCanEnter(fluidType) != 0; }
	// Move less dense fluids to their group's excessVolume until Config::maxBlockVolume is achieved.
	void resolveFluidOverfull();

	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(bool exposed);
	void setBelowExposedToSky();
	void setBelowNotExposedToSky();

	std::vector<Block*> selectBetweenCorners(Block* otherBlock) const;

	bool operator==(const Block& block) const;
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
			for(Block* adjacent : block->m_adjacentsVector)
				if(condition(*adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	std::unordered_set<Block*> collectAdjacentsInRange(uint32_t range);
	std::vector<Block*> collectAdjacentsInRangeVector(uint32_t range);
};
