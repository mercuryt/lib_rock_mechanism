#include "findsPath.h"
#include "hasShape.h"
#include "types.h"
#include "area.h"
#include <utility>
FindsPath::FindsPath(const HasShape& hs, bool detour) : 
	m_hasShape(hs), m_detour(detour) { }
void FindsPath::pathToBlock(BlockIndex destination)
{
	std::function<bool(BlockIndex, Facing)> predicate = [&](BlockIndex location, [[maybe_unused]] Facing facing) { return location == destination; };
	pathToPredicateWithHuristicDestination(predicate, destination);
}
void FindsPath::pathAdjacentToBlock(BlockIndex target)
{
	std::function<bool(BlockIndex, Facing facing)> predicate = [&](BlockIndex location, Facing facing)
	{
		std::function<bool(BlockIndex)> isTarget = [&](BlockIndex adjacent) { return adjacent == target; };
		BlockIndex adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, isTarget);
		if(adjacent == BLOCK_INDEX_MAX)
			return false;
		m_target = adjacent;
		return true;
	};
	pathToPredicateWithHuristicDestination(predicate, target);
}
void FindsPath::pathToAdjacentToPredicate(std::function<bool(BlockIndex)>& predicate)
{
	std::function<bool(BlockIndex, Facing)> condition = [&](BlockIndex location, Facing facing)
	{
		BlockIndex adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
		if(adjacent == BLOCK_INDEX_MAX)
			return false;
		m_target = adjacent;
		return true;
	};
	if(m_huristicDestination == BLOCK_INDEX_MAX)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, m_huristicDestination);
}
void FindsPath::pathToOccupiedPredicate(std::function<bool(BlockIndex)>& predicate)
{
	std::function<bool(BlockIndex, Facing)> condition = [&](BlockIndex location, Facing facing)
	{
		BlockIndex adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(location, facing, predicate);
		if(adjacent == BLOCK_INDEX_MAX)
			return false;
		m_target = adjacent;
		return true;
	};
	if(m_huristicDestination == BLOCK_INDEX_MAX)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, m_huristicDestination);
}
void FindsPath::pathToUnreservedAdjacentToPredicate(std::function<bool(BlockIndex)>& predicate, Faction& faction)
{
	std::function<bool(BlockIndex, Facing)> condition = [&](BlockIndex location, Facing facing)
	{
		if(!m_hasShape.allBlocksAtLocationAndFacingAreReservable(location, facing, faction))
			return false;
		BlockIndex adjacent = const_cast<HasShape&>(m_hasShape).getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
		if(adjacent == BLOCK_INDEX_MAX)
			return false;
		m_target = adjacent;
		return true;
	};
	if(m_huristicDestination == BLOCK_INDEX_MAX)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, m_huristicDestination);
}
void FindsPath::pathToUnreservedPredicate(std::function<bool(BlockIndex)>& predicate, Faction& faction)
{
	std::function<bool(BlockIndex, Facing)> condition = [&](BlockIndex location, Facing facing)
	{
		if(!m_hasShape.allBlocksAtLocationAndFacingAreReservable(location, facing, faction))
			return false;
		BlockIndex occupied = const_cast<HasShape&>(m_hasShape).getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(location, facing, predicate);
		return occupied != BLOCK_INDEX_MAX;
	};
	if(m_huristicDestination == BLOCK_INDEX_MAX)
		pathToPredicate(condition);
	else
		pathToPredicateWithHuristicDestination(condition, m_huristicDestination);
}
void FindsPath::pathToUnreservedAdjacentToHasShape(const HasShape& otherShape, Faction& faction)
{
	m_route = m_hasShape.m_area->m_hasTerrainFacades.at(m_hasShape.getMoveType()).findPathAdjacentToAndUnreserved(m_hasShape.m_location, *m_hasShape.m_shape, otherShape, faction, m_detour);
}
void FindsPath::pathToAreaEdge()
{
	Blocks& blocks = m_hasShape.m_area->getBlocks();
	std::function<bool(BlockIndex, Facing)> predicate = [&](BlockIndex location, Facing facing)
	{
		for(BlockIndex occupied : const_cast<HasShape&>(m_hasShape).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<BlockIndex&>(location), facing))
			if(blocks.isEdge(occupied))
				return true;
		return false;
	};
	pathToPredicate(predicate);
}
void FindsPath::pathToPredicate(DestinationCondition& predicate)
{
	m_route = m_hasShape.m_area->m_hasTerrainFacades.at(m_hasShape.getMoveType()).findPathToCondition(m_hasShape.m_location, *m_hasShape.m_shape, predicate, m_detour);
}
void FindsPath::pathToPredicateWithHuristicDestination(DestinationCondition& predicate, BlockIndex huristicDestination)
{
	m_route = m_hasShape.m_area->m_hasTerrainFacades.at(m_hasShape.getMoveType()).findPathToCondition(m_hasShape.m_location, *m_hasShape.m_shape, predicate, huristicDestination, m_detour);
}
std::vector<BlockIndex> FindsPath::getOccupiedBlocksAtEndOfPath()
{
	return const_cast<HasShape&>(m_hasShape).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(m_route.back(), getFacingAtDestination());
}
std::vector<BlockIndex> FindsPath::getAdjacentBlocksAtEndOfPath()
{
	return const_cast<HasShape&>(m_hasShape).getAdjacentAtLocationWithFacing(m_route.back(), getFacingAtDestination());
}
Facing FindsPath::getFacingAtDestination() const
{
	assert(!m_route.empty());
	Blocks& blocks = m_hasShape.m_area->getBlocks();
	BlockIndex secondToLast = m_route.size() == 1 ? m_hasShape.m_location : m_route.at(m_route.size() - 1);
	return blocks.facingToSetWhenEnteringFrom(m_route.back(), secondToLast);
}
bool FindsPath::areAllBlocksAtDestinationReservable(Faction* faction) const
{
	if(faction ==  nullptr)
		return true;
	if(m_route.empty())
	{
		assert(m_useCurrentLocation);
		return m_hasShape.allBlocksAtLocationAndFacingAreReservable(m_hasShape.m_location, m_hasShape.m_facing, *faction);
	}
	return m_hasShape.allBlocksAtLocationAndFacingAreReservable(m_route.back(), getFacingAtDestination(), *faction);
}
void FindsPath::reserveBlocksAtDestination(CanReserve& canReserve)
{
	Blocks& blocks = m_hasShape.m_area->getBlocks();
	if(!canReserve.hasFaction())
		return;
	for(BlockIndex occupied : getOccupiedBlocksAtEndOfPath())
		blocks.reserve(occupied, canReserve);
}
void FindsPath::reset()
{
	m_target = BLOCK_INDEX_MAX;
	m_route.clear();
}
