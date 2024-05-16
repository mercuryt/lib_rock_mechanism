#pragma once

#include "../types.h"
#include "../../lib/dynamic_bitset.hpp"
#include <TGUI/Animation.hpp>

struct MoveType;
class Area;
class Block;
class Actor;
class TerrainFacade;

constexpr int maxAdjacent = 26;
using DestinationCondition = std::function<bool(BlockIndex)>;
using AccessCondition = std::function<bool(BlockIndex, BlockIndex, uint8_t)>;

class TerrainFacade final
{
	Area& m_area;
	const MoveType& m_moveType;
	sul::dynamic_bitset<> m_enterable;
	// DestinationCondition could test against a set of destination indecies or load the actual block to do more complex checks.
	// AccessCondition could test for larger shapes or just return true for 1x1x1 size.
	// TODO: huristic destination.
	[[nodiscard]] std::vector<Block*> findPath(BlockIndex from, const DestinationCondition destinationCondition, AccessCondition accessCondition);
	[[nodiscard]] bool canEnterFrom(BlockIndex blockIndex, uint8_t adjacentIndex) const;
	[[nodiscard]] std::vector<Block*> findPathToAnyOfForSingleBlockActor(BlockIndex start, std::vector<BlockIndex> indecies);
	[[nodiscard]] std::vector<Block*> findPathToAnyOfForMultiBlockActor(const Actor& actor, std::vector<BlockIndex> indecies);
	[[nodiscard]] std::vector<Block*> findPathToConditionForSingleBlockActor(BlockIndex start, const DestinationCondition destinationCondition);
	[[nodiscard]] std::vector<Block*> findPathToConditionForMultiBlockActor(const Actor& actor, const DestinationCondition destinationCondition);
public:
	TerrainFacade(Area& area, const MoveType& moveType);
	void initalize();
	void update(const Block& block);
	[[nodiscard]] std::vector<Block*> findPathToAnyOf(const Actor& actor, std::vector<BlockIndex> indecies);
	[[nodiscard]] std::vector<Block*> findPathToCondition(const Actor& actor, const DestinationCondition destinationCondition);
};
