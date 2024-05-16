/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Should only be used as inheritance target by Block, not intended to ever be instanced.
 */
#pragma once

#include "types.h"
#include "blockFeature.h"
#include "designations.h"
#include "stockpile.h"
#include "farmFields.h"
#include "temperature.h"
#include "fire.h"
#include "block/hasActors.h"
#include "block/hasFluids.h"
#include "block/hasItems.h"
#include "block/hasPlant.h"
#include "block/hasShapes.h"

#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <array>
#include <stack>
#include <vector>
#include <string>

class Area;
struct DeserializationMemo;
struct MoveType;
struct VisionCuboid;
struct LocationBucket;
//class Item;
//class Actor;

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
	// Store the location bucket this block belongs to.
	LocationBucket* m_locationBucket;
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
	BlockHasFluids m_hasFluids;
	Reservable m_reservable;
	HasPlant m_hasPlant;
	HasBlockFeatures m_hasBlockFeatures;
	BlockHasActors m_hasActors;
	BlockHasItems m_hasItems;
	HasDesignations m_hasDesignations;
	BlockIsPartOfStockPiles m_isPartOfStockPiles;
	IsPartOfFarmField m_isPartOfFarmField;
	BlockHasTemperature m_blockHasTemperature;
	BlockHasProjects m_hasProjects;

	Block();
	Block(const Block&) = delete;
	Block(Block&&) = delete;
	Block& operator=(const Block&) = delete;
	void setup(Area& area, DistanceInBlocks ax, DistanceInBlocks ay, DistanceInBlocks az);
	void recordAdjacent();
	[[nodiscard]] std::vector<Block*> getAdjacentWithEdgeAdjacent() const;
	[[nodiscard]] std::vector<Block*> getAdjacentWithEdgeAndCornerAdjacent() const;
	[[nodiscard]] std::vector<Block*> getEdgeAndCornerAdjacentOnly() const;
	[[nodiscard]] std::vector<Block*> getEdgeAdjacentOnly() const;
	[[nodiscard]] std::vector<Block*> getEdgeAdjacentOnSameZLevelOnly() const;
	[[nodiscard]] std::vector<Block*> getAdjacentOnSameZLevelOnly() const;
	[[nodiscard]] std::vector<Block*> getEdgeAdjacentOnlyOnNextZLevelDown() const;
	[[nodiscard]] std::vector<Block*> getEdgeAndCornerAdjacentOnlyOnNextZLevelDown() const;
	[[nodiscard]] std::vector<Block*> getEdgeAdjacentOnlyOnNextZLevelUp() const;
	[[nodiscard]] DistanceInBlocks distance(Block& block) const;
	[[nodiscard]] DistanceInBlocks taxiDistance(const Block& block) const;
	[[nodiscard]] bool squareOfDistanceIsMoreThen(const Block& block, DistanceInBlocks distanceSquared) const;
	[[nodiscard]] bool isAdjacentToAny(std::unordered_set<Block*>& blocks) const;
	[[nodiscard]] bool isAdjacentTo(Block& block) const;
	[[nodiscard]] bool isAdjacentToIncludingCornersAndEdges(Block& block) const;
	[[nodiscard]] bool isAdjacentTo(HasShape& hasShape) const;
	void setNotSolid();
	void setSolid(const MaterialType& materialType, bool contructed = false);
	[[nodiscard]] bool isSolid() const;
	[[nodiscard]] const MaterialType& getSolidMaterial() const;
	[[nodiscard]] bool isConstructed() const { return m_constructed; }
	[[nodiscard]] bool canSeeIntoFromAlways(const Block& block) const;
	void moveContentsTo(Block& block);
	[[nodiscard]] Mass getMass() const;
	// Get block at offset coordinates. Can return nullptr.
	[[nodiscard]] Block* offset(int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] Block& offsetNotNull(int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] std::array<int32_t, 3> relativeOffsetTo(const Block& block) const; 
	[[nodiscard]] bool canSeeThrough() const;
	[[nodiscard]] bool canSeeThroughFloor() const;
	[[nodiscard]] bool canSeeThroughFrom(const Block& block) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFrom(const Block& block) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFromIncludingDiagonal(const Block& block, Facing inital = 0) const;
	[[nodiscard]] bool isSupport() const;
	[[nodiscard]] bool hasLineOfSightTo(Block& block) const;
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	// TODO: Is this being used?
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEver(const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] Block* getBlockBelow() { return m_adjacents[0]; }
	[[nodiscard]] const Block* getBlockBelow() const { return m_adjacents[0]; }
	[[nodiscard]] Block* getBlockAbove() { return m_adjacents[5]; }
	[[nodiscard]] const Block* getBlockAbove() const { return m_adjacents[5]; }
	[[nodiscard]] Block* getBlockNorth() { return m_adjacents[1]; }
	[[nodiscard]] const Block* getBlockNorth() const { return m_adjacents[1]; }
	[[nodiscard]] Block* getBlockWest() { return m_adjacents[2]; }
	[[nodiscard]] const Block* getBlockWest() const { return m_adjacents[2]; }
	[[nodiscard]] Block* getBlockSouth() { return m_adjacents[3]; }
	[[nodiscard]] const Block* getBlockSouth() const { return m_adjacents[3]; }
	[[nodiscard]] Block* getBlockEast() { return m_adjacents[4]; }
	[[nodiscard]] const Block* getBlockEast() const { return m_adjacents[4]; }
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(bool exposed);
	void setBelowExposedToSky();
	void setBelowNotExposedToSky();
	void setBelowVisible();
	[[nodiscard]] std::vector<Block*> selectBetweenCorners(Block* otherBlock) const;
	[[nodiscard]] bool operator==(const Block& block) const;
	//TODO: Use std::function instead of template.
	template <typename F>
	[[nodiscard]] std::unordered_set<Block*> collectAdjacentsWithCondition(F&& condition)
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
	[[nodiscard]] Block* getBlockInRangeWithCondition(DistanceInBlocks range, F&& condition)
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
	[[nodiscard]] std::unordered_set<Block*> collectAdjacentsInRange(DistanceInBlocks range);
	[[nodiscard]] std::vector<Block*> collectAdjacentsInRangeVector(DistanceInBlocks range);
	void loadFromJson(Json data, DeserializationMemo& deserializationMemo, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Json positionToJson() const;
	static inline const int32_t offsetsListAllAdjacent[26][3] = {
		{-1,1,-1}, {-1,0,-1}, {-1,-1,-1},
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,1,-1}, {1,0,-1}, {1,-1,-1},

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {-1,1,0},

		{-1,1,1}, {-1,0,1}, {-1,-1,1},
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,1,1}, {1,0,1}, {1,-1,1}
	};
};
inline void to_json(Json& data, const Block* const& block){ data = block->positionToJson(); }
inline void to_json(Json& data, const Block& block){ data = block.positionToJson(); }
inline void to_json(Json& data, const std::unordered_set<Block*>& blocks)
{
	for(const Block* block : blocks)
		data.push_back(block);
}
