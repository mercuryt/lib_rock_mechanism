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
	[[nodiscard]] std::vector<BlockIndex> findPathToForSingleBlockShape(BlockIndex start, BlockIndex target, bool detour = false, CollisionVolume volume = 0) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToForMultiBlockShape(BlockIndex start, const Shape& shape, BlockIndex target, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToAnyOfForSingleBlockShape(BlockIndex start, std::vector<BlockIndex> indecies, BlockIndex huristincDestination, bool detour = false, CollisionVolume volume = 0) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToAnyOfForMultiBlockShape(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristincDestination, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToConditionForSingleBlockShape(BlockIndex start, const DestinationCondition destinationCondition, BlockIndex huristincDestination, bool detour = false, CollisionVolume volume = 0) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToConditionForMultiBlockShape(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristincDestination, bool detour = false) const;
	bool getValueForBit(BlockIndex from, BlockIndex to) const;
public:
	TerrainFacade(Area& area, const MoveType& moveType);
	void update(BlockIndex block);
	[[nodiscard]] std::vector<BlockIndex> findPathTo(BlockIndex start, const Shape& shape, BlockIndex target, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToAnyOf(BlockIndex start, const Shape& shape, std::vector<BlockIndex> indecies, BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathToCondition(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentTo(BlockIndex start, const Shape& shape, BlockIndex target, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentTo(BlockIndex start, const Shape& shape, const HasShape& hasShape, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, BlockIndex target, const Faction& faction, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentToAndUnreserved(BlockIndex start, const Shape& shape, const HasShape& hasShape, const Faction& faction, bool detour = false) const;
	[[nodiscard]] std::vector<BlockIndex> findPathAdjacentToCondition(BlockIndex start, const Shape& shape, const DestinationCondition destinationCondition, BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool detour = false) const;
};

class AreaHasTerrainFacades
{
	std::unordered_map<const MoveType*, TerrainFacade> m_data;
	Area& m_area;
	void update(BlockIndex block);
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void updateBlockAndAdjacent(BlockIndex block);
	TerrainFacade& at(const MoveType&);
	void maybeRegisterMoveType(const MoveType& moveType);
};
