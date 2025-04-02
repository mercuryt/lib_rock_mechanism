#pragma once
#include "geometry/cuboidSet.h"
#include "strongInteger.h"
#include "types.h"
#include "../lib/json.hpp"

class AreaHasDecks
{
	SmallMap<DeckId, CuboidSet> m_data;
	StrongVector<DeckId, BlockIndex> m_blockData;
	DeckId m_nextId = DeckId::create(0);
public:
	AreaHasDecks(Area& area);
	void updateBlocks(Area& area, const DeckId& id);
	void clearBlocks(Area& area, const DeckId& id);
	[[nodiscard]] DeckId registerDecks(Area& area, CuboidSet decks);
	void unregisterDecks(Area& area, const DeckId& id);
	void shift(Area& area, const DeckId& id, const Offset3D& offset, const DistanceInBlocks& distance, const BlockIndex& origin, const Facing4& oldFacing, const Facing4& newFacing);
	[[nodiscard]] bool blockIsPartOfDeck(const BlockIndex& block) const { return m_blockData[block].exists(); }
	[[nodiscard]] DeckId getForBlock(const BlockIndex& block) const { return m_blockData[block]; }
};