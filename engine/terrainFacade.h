#pragma once

#include "types.h"
#include "../lib/dynamic_bitset.hpp"

struct MoveType;
struct Shape;
struct Faction;
class Area;
class TerrainFacade;
class HasShape;

constexpr int maxAdjacent = 26;
// TODO: optimization: single block shapes don't need facing

class TerrainFacade final
{
	sul::dynamic_bitset<> m_enterable;
	Area& m_area;
	const MoveType& m_moveType;
	// DestinationCondition could test against a set of destination indecies or load the actual block to do more complex checks.
	// AccessCondition could test for larger shapes or just return true for 1x1x1 size.
	// TODO: huristic destination.
	[[nodiscard]] std::vector<BlockIndex> findPath(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, OpenListPush openListPush, OpenListPop openListPop, OpenListEmpty openListEmpty) const;
	[[nodiscard]] std::vector<BlockIndex> findPathBreathFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition) const;
	[[nodiscard]] std::vector<BlockIndex> findPathDepthFirst(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition, BlockIndex huristicDestination) const;
	[[nodiscard]] bool canEnterFrom(BlockIndex blockIndex, uint8_t adjacentIndex) const;
	[[nodiscard]] bool shapeCanFitWithFacing(BlockIndex blockIndex, const Shape& shape, Facing facing) const;
	[[nodiscard]] bool shapeCanFitWithAnyFacing(BlockIndex blockIndex, const Shape& shape) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToForSingleBlockShape(BlockIndex start, BlockIndex target) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, BlockIndex target) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToAnyOfForSingleBlockShape(BlockIndex start, std::vector<BlockIndex> indecies, BlockIndex huristincDestination) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristincDestination) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToConditionForSingleBlockShape(BlockIndex start, const DestinationCondition destinationCondition, BlockIndex huristincDestination) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristincDestination) const;
public:
	TerrainFacade(Area& area, const MoveType& moveType);
	void initalize();
	void update(BlockIndex block);
	[[nodiscard]] std::vector<BlockIndex> findPathTo(BlockIndex start, const Shape& shape, BlockIndex target) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToAnyOf(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination = BLOCK_INDEX_MAX) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToCondition(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination = BLOCK_INDEX_MAX) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentTo(BlockIndex start, const Shape& shape, BlockIndex target) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentTo(BlockIndex start, const Shape& shape, const HasShape& hasShape) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, BlockIndex target, const Faction& faction) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, const HasShape& hasShape, const Faction& faction) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentToCondition(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination = BLOCK_INDEX_MAX) const;
};

class AreaHasTerrainFacades
{
	std::unordered_map<const MoveType*, TerrainFacade> m_data;
	Area& m_area;
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void update(BlockIndex block);
	TerrainFacade& at(const MoveType&);
};
