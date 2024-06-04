/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Should only be used as inheritance target by BlockIndex, not intended to ever be instanced.
 */
#pragma once

#include "types.h"
#include "blockFeature.h"
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
enum class BlockDesignation;
//class Item;
//class Actor;

class BlockIndex final
{
public:
	Reservable m_reservable; // 6.5 sets of 64 bits
	// Store adjacent in an array, with index determined by relative position.
	// below = 0, < = 1, ^ = 2, > = 3, v = 4, above = 5.
	std::array<BlockIndex*, 6> m_adjacents; // 6
	BlockHasFluids m_hasFluids; // 6
	BlockHasShapes m_hasShapes; // 6
	BlockHasActors m_hasActors; // 3
	BlockHasItems m_hasItems; // 3
	BlockIsPartOfStockPiles m_isPartOfStockPiles; // 3
	BlockIsPartOfFarmField m_isPartOfFarmField; // 3
	HasBlockFeatures m_hasBlockFeatures; // 2
	BlockHasProjects m_hasProjects; // 2
	BlockHasPlant m_hasPlant; //2
	std::unordered_map<const MaterialType* , Fire>* m_fires = nullptr;
	BlockHasTemperature m_blockHasTemperature; // 1.5
	// Store area as a pointer rather then a reference to keep block default constructable.
	// This is required inorder to create the m_blocks structure before initalizing it.
	// TODO: emplace_back?
	Area* m_area = nullptr;
private:
	// If this block is solid stone, solid dirt, etc. then store it here. Otherwise nullptr.
	const MaterialType* m_solid = nullptr;
public:
	// Store the location bucket this block belongs to.
	LocationBucket* m_locationBucket = nullptr;
	DistanceInBlocks m_x = 0;
	DistanceInBlocks m_y = 0;
	DistanceInBlocks m_z = 0;
	bool m_exposedToSky = true;
	bool m_underground = false;
	bool m_isEdge = false;
	bool m_outdoors = true;
	bool m_visible = true;
private:
	bool m_constructed = false;
public:
	BlockIndex();
	BlockIndex(const BlockIndex&) = delete;
	BlockIndex(BlockIndex&&) = delete;
	BlockIndex& operator=(const BlockIndex&) = delete;
	void setup(Area& area, DistanceInBlocks ax, DistanceInBlocks ay, DistanceInBlocks az);
	void recordAdjacent();
	void setDesignation(Faction& faction, BlockDesignation designation);
	void unsetDesignation(Faction& faction, BlockDesignation designation);
	void maybeUnsetDesignation(Faction& faction, BlockDesignation designation);
	[[nodiscard]] BlockIndex getIndex() const;
	[[nodiscard]] bool hasDesignation(Faction& faction, BlockDesignation designation) const;
	[[nodiscard]] std::vector<BlockIndex*> getAdjacentWithEdgeAdjacent() const;
	[[nodiscard]] std::vector<BlockIndex*> getAdjacentWithEdgeAndCornerAdjacent() const;
	[[nodiscard]] std::vector<BlockIndex*> getEdgeAndCornerAdjacentOnly() const;
	[[nodiscard]] std::vector<BlockIndex*> getEdgeAdjacentOnly() const;
	[[nodiscard]] std::vector<BlockIndex*> getEdgeAdjacentOnSameZLevelOnly() const;
	[[nodiscard]] std::vector<BlockIndex*> getAdjacentOnSameZLevelOnly() const;
	[[nodiscard]] std::vector<BlockIndex*> getEdgeAdjacentOnlyOnNextZLevelDown() const;
	[[nodiscard]] std::vector<BlockIndex*> getEdgeAndCornerAdjacentOnlyOnNextZLevelDown() const;
	[[nodiscard]] std::vector<BlockIndex*> getEdgeAdjacentOnlyOnNextZLevelUp() const;
	[[nodiscard]] DistanceInBlocks distance(BlockIndex& block) const;
	[[nodiscard]] DistanceInBlocks taxiDistance(const BlockIndex& block) const;
	[[nodiscard]] bool squareOfDistanceIsMoreThen(const BlockIndex& block, DistanceInBlocks distanceSquared) const;
	[[nodiscard]] bool isAdjacentToAny(std::unordered_set<BlockIndex*>& blocks) const;
	[[nodiscard]] bool isAdjacentTo(BlockIndex& block) const;
	[[nodiscard]] bool isAdjacentToIncludingCornersAndEdges(BlockIndex& block) const;
	[[nodiscard]] bool isAdjacentTo(HasShape& hasShape) const;
	void setNotSolid();
	void setSolid(const MaterialType& materialType, bool contructed = false);
	[[nodiscard]] bool isSolid() const;
	[[nodiscard]] const MaterialType& getSolidMaterial() const;
	[[nodiscard]] bool isConstructed() const { return m_constructed; }
	[[nodiscard]] bool canSeeIntoFromAlways(const BlockIndex& block) const;
	void moveContentsTo(BlockIndex& block);
	[[nodiscard]] Mass getMass() const;
	// Get block at offset coordinates. Can return nullptr.
	[[nodiscard]] BlockIndex* offset(int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] BlockIndex& offsetNotNull(int32_t ax, int32_t ay, int32_t az) const;
	[[nodiscard]] std::array<int32_t, 3> relativeOffsetTo(const BlockIndex& block) const; 
	[[nodiscard]] bool canSeeThrough() const;
	[[nodiscard]] bool canSeeThroughFloor() const;
	[[nodiscard]] bool canSeeThroughFrom(const BlockIndex& block) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFrom(const BlockIndex& block) const;
	[[nodiscard]] Facing facingToSetWhenEnteringFromIncludingDiagonal(const BlockIndex& block, Facing inital = 0) const;
	[[nodiscard]] bool isSupport() const;
	[[nodiscard]] bool hasLineOfSightTo(BlockIndex& block) const;
	// Validate the nongeneric object can enter this block and also any other blocks required by it's Shape comparing to m_totalStaticVolume.
	// TODO: Is this being used?
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEver(const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] BlockIndex* getBlockBelow() { return m_adjacents[0]; }
	[[nodiscard]] const BlockIndex* getBlockBelow() const { return m_adjacents[0]; }
	[[nodiscard]] BlockIndex* getBlockAbove() { return m_adjacents[5]; }
	[[nodiscard]] const BlockIndex* getBlockAbove() const { return m_adjacents[5]; }
	[[nodiscard]] BlockIndex* getBlockNorth() { return m_adjacents[1]; }
	[[nodiscard]] const BlockIndex* getBlockNorth() const { return m_adjacents[1]; }
	[[nodiscard]] BlockIndex* getBlockWest() { return m_adjacents[2]; }
	[[nodiscard]] const BlockIndex* getBlockWest() const { return m_adjacents[2]; }
	[[nodiscard]] BlockIndex* getBlockSouth() { return m_adjacents[3]; }
	[[nodiscard]] const BlockIndex* getBlockSouth() const { return m_adjacents[3]; }
	[[nodiscard]] BlockIndex* getBlockEast() { return m_adjacents[4]; }
	[[nodiscard]] const BlockIndex* getBlockEast() const { return m_adjacents[4]; }
	// Called from setSolid / setNotSolid as well as from user code such as construct / remove floor.
	void setExposedToSky(bool exposed);
	void setBelowExposedToSky();
	void setBelowNotExposedToSky();
	void setBelowVisible();
	[[nodiscard]] std::vector<BlockIndex*> selectBetweenCorners(BlockIndex* otherBlock) const;
	[[nodiscard]] bool operator==(const BlockIndex& block) const;
	//TODO: Use std::function instead of template.
	template <typename F>
	[[nodiscard]] std::unordered_set<BlockIndex*> collectAdjacentsWithCondition(F&& condition)
	{
		std::unordered_set<BlockIndex*> output;
		std::stack<BlockIndex*> openList;
		openList.push(this);
		output.insert(this);
		while(!openList.empty())
		{
			BlockIndex* block = openList.top();
			openList.pop();
			for(BlockIndex* adjacent : block->m_adjacents)
				if(adjacent && condition(*adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	template <typename F>
	[[nodiscard]] BlockIndex* getBlockInRangeWithCondition(DistanceInBlocks range, F&& condition)
	{
		std::stack<BlockIndex*> open;
		open.push(this);
		std::unordered_set<BlockIndex*> closed;
		while(!open.empty())
		{
			BlockIndex* block = open.top();
			if(condition(*block))
				return block;
			open.pop();
			for(BlockIndex* adjacent : block->m_adjacents)
				if(adjacent && taxiDistance(*adjacent) <= range && !closed.contains(adjacent))
				{
					closed.insert(adjacent);
					open.push(adjacent);
				}
					
				
		}
		return nullptr;
	}
	[[nodiscard]] std::unordered_set<BlockIndex*> collectAdjacentsInRange(DistanceInBlocks range);
	[[nodiscard]] std::vector<BlockIndex*> collectAdjacentsInRangeVector(DistanceInBlocks range);
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
inline void to_json(Json& data, const BlockIndex* const& block){ data = block->positionToJson(); }
inline void to_json(Json& data, const BlockIndex& block){ data = block.positionToJson(); }
inline void to_json(Json& data, const std::unordered_set<BlockIndex*>& blocks)
{
	for(const BlockIndex* block : blocks)
		data.push_back(block);
}
