#include "deck.h"
#include "area/area.h"
#include "blocks/blocks.h"
AreaHasDecks::AreaHasDecks(Area& area)
{
	m_blockData.resize(area.getBlocks().size());
}
void AreaHasDecks::updateBlocks(Area& area, const DeckId& id)
{
	for(const BlockIndex& block : m_data[id].getView(area.getBlocks()))
	{
		assert(m_blockData[block].empty());
		m_blockData[block] = id;
		// TODO: bulk update.
		area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
	}
}
void AreaHasDecks::clearBlocks(Area& area, const DeckId& id)
{
	for(const BlockIndex& block : m_data[id].getView(area.getBlocks()))
		m_blockData[block].clear();
}
[[nodiscard]] DeckId AreaHasDecks::registerDecks(Area& area, CuboidSet decks)
{
	m_data.emplace(m_nextId, decks);
	updateBlocks(area, m_nextId);
	++m_nextId;
	return m_nextId;
}
void AreaHasDecks::unregisterDecks(Area& area, const DeckId& id)
{
	clearBlocks(area, id);
	m_data.erase(id);
}
void AreaHasDecks::shift(Area& area, const DeckId& id, const Offset3D& offset, const DistanceInBlocks& distance, const BlockIndex& origin, const Facing4& oldFacing, const Facing4& newFacing)
{
	clearBlocks(area, id);
	if(oldFacing != newFacing)
	{
		int facingChange = (int)oldFacing - (int)newFacing;
		if(facingChange < 0)
			facingChange += 4;
		m_data[id].rotateAroundPoint(area.getBlocks(), origin, (Facing4)facingChange);
	}
	m_data[id].shift(offset, distance);
	updateBlocks(area, id);
}