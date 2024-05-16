#include "terrainFacade.h"
#include "actor.h"
#include "area.h"
#include "block.h"
#include "types.h"
#include "actor.h"
#include <unordered_map>

TerrainFacade::TerrainFacade(Area& area, const MoveType& moveType) : m_area(area), m_moveType(moveType)
{
	m_enterable.resize(m_area.getBlocks().size() * maxAdjacent);
}
void TerrainFacade::initalize()
{
	for(const Block& block : m_area.getBlocks())
		update(block);
}
void TerrainFacade::update(const Block& block)
{
	BlockIndex index = m_area.getBlockIndex(block);
	std::vector<Block*> adjacent = block.getAdjacentWithEdgeAndCornerAdjacent();
	for(int adjacentIndex = 0; adjacentIndex < maxAdjacent; ++adjacentIndex)
		m_enterable[index + adjacentIndex] = adjacent[adjacentIndex]->m_hasShapes.moveTypeCanEnterFrom(m_moveType, block);
}
std::vector<Block*> TerrainFacade::findPath(BlockIndex current, const DestinationCondition destinationCondition, const AccessCondition accessCondition)
{
	std::unordered_map<BlockIndex, BlockIndex> closed;
	std::deque<BlockIndex> open;
	closed[current] = BLOCK_INDEX_MAX;
	open.push_back(current);
	while(!open.empty())
	{
		current = open.back();
		for(uint8_t adjacentIndex = 0; adjacentIndex < maxAdjacent; ++adjacentIndex)
			if(canEnterFrom(current, adjacentIndex) && !closed.contains(current) && accessCondition)
			{
				int32_t blockIndexOffsetForAdjacentIndex = m_area.indexOffsetForAdjacentOffset(adjacentIndex);
				if(destinationCondition(current + blockIndexOffsetForAdjacentIndex))
				{
					std::vector<Block*> output;
					output.push_back(&m_area.getBlocks()[current + blockIndexOffsetForAdjacentIndex]);
					while(current != BLOCK_INDEX_MAX)
					{
						output.push_back(&m_area.getBlocks()[current]);
						current = closed[current];
					}
					std::reverse(output.begin(), output.end());
					return output;
				}
				BlockIndex next = current + blockIndexOffsetForAdjacentIndex;
				closed[next] = current;
				open.push_back(current);
			}
		open.pop_back();
	}
	return {};
}
bool TerrainFacade::canEnterFrom(BlockIndex blockIndex, uint8_t adjacentIndex) const
{
	return m_enterable[(blockIndex * maxAdjacent) + adjacentIndex];
}
std::vector<Block*> TerrainFacade::findPathToAnyOfForSingleBlockActor(BlockIndex start, std::vector<BlockIndex> indecies)
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index){ return std::ranges::find(indecies, index) != indecies.end(); };
	AccessCondition accessCondition = [](BlockIndex, BlockIndex, uint8_t){ return true; };
	return findPath(start, destinationCondition, accessCondition);
}
std::vector<Block*> TerrainFacade::findPathToAnyOfForMultiBlockActor([[maybe_unused]] const Actor& actor, [[maybe_unused]] std::vector<BlockIndex> indecies)
{
	DestinationCondition destinationCondition = [indecies](BlockIndex index){ return std::ranges::find(indecies, index) != indecies.end(); };
	AccessCondition accessCondition = [this, &actor]([[maybe_unused]] BlockIndex from, BlockIndex to, uint8_t adjacentOffset){
		Facing facing = m_area.getFacingForAdjacentOffset(adjacentOffset);
		for(auto[x, y, z, v] : actor.m_shape->positionsWithFacing(facing))
		{
			BlockIndex wouldBeOccupiedIndex = to + m_area.indexOffsetForCoordinateOffset(x, y, z);
			if(!m_enterable[wouldBeOccupiedIndex])
				return false;
		}
		return true;
	};
	return findPath(m_area.getBlockIndex(*actor.m_location), destinationCondition, accessCondition);
}
std::vector<Block*> TerrainFacade::findPathToConditionForSingleBlockActor(BlockIndex start, const DestinationCondition destinationCondition)
{
	AccessCondition accessCondition = [](BlockIndex, BlockIndex, uint8_t){ return true; };
	return findPath(start, destinationCondition, accessCondition);
}
std::vector<Block*> TerrainFacade::findPathToConditionForMultiBlockActor([[maybe_unused]] const Actor& actor, [[maybe_unused]] const DestinationCondition destinationCondition)
{

	AccessCondition accessCondition = [this, &actor]([[maybe_unused]] BlockIndex from, BlockIndex to, uint8_t adjacentOffset){
		Facing facing = m_area.getFacingForAdjacentOffset(adjacentOffset);
		for(auto[x, y, z, v] : actor.m_shape->positionsWithFacing(facing))
		{
			BlockIndex wouldBeOccupiedIndex = to + m_area.indexOffsetForCoordinateOffset(x, y, z);
			if(!m_enterable[wouldBeOccupiedIndex])
				return false;
		}
		return true;
	};
	return findPath(m_area.getBlockIndex(*actor.m_location), destinationCondition, accessCondition);
}
std::vector<Block*> TerrainFacade::findPathToAnyOf(const Actor& actor, std::vector<BlockIndex> indecies)
{
	assert(actor.m_canMove.getMoveType() == m_moveType);
	if(actor.m_shape->isMultiTile)
		return findPathToAnyOfForMultiBlockActor(actor, indecies);
	else
		return findPathToAnyOfForSingleBlockActor(m_area.getBlockIndex(*actor.m_location), indecies);
}
std::vector<Block*> TerrainFacade::findPathToCondition(const Actor& actor, const std::function<bool(BlockIndex)> destinationCondition)
{
	assert(actor.m_canMove.getMoveType() == m_moveType);
	if(actor.m_shape->isMultiTile)
		return findPathToConditionForMultiBlockActor(actor, destinationCondition);
	else
		return findPathToConditionForSingleBlockActor(m_area.getBlockIndex(*actor.m_location), destinationCondition);
}
