#include "actors.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
#include "../reservable.hpp"
void Actors::canReserve_clearAll(const ActorIndex& index)
{
	m_canReserve[index]->deleteAllWithoutCallback();
}
void Actors::canReserve_setFaction(const ActorIndex& index, FactionId faction)
{
	m_canReserve[index]->setFaction(faction);
}
void Actors::canReserve_reserveLocation(const ActorIndex& index, const BlockIndex& block, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	m_area.getBlocks().reserve(block, canReserve_get(index), std::move(callback));
}
void Actors::canReserve_reserveItem(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	m_area.getItems().reservable_reserve(item, canReserve_get(index), quantity, std::move(callback));
}
bool Actors::canReserve_translateAndReservePositions(const ActorIndex& index, SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>>&& blocksAndCallbacks, const BlockIndex& prevousPivot, const BlockIndex& newPivot, const Facing4& previousFacing, const Facing4& newFacing)
{
	return m_canReserve[index]->translateAndReservePositions(m_area.getBlocks(), std::move(blocksAndCallbacks), prevousPivot, newPivot, previousFacing, newFacing);
}
bool Actors::canReserve_tryToReserveLocation(const ActorIndex& index, const BlockIndex& block, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	if(m_area.getBlocks().isReserved(block, getFaction(index)))
		return false;
	canReserve_reserveLocation(index, block, std::move(callback));
	return true;
}
bool Actors::canReserve_tryToReserveItem(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	if(m_area.getItems().reservable_isFullyReserved(item, m_faction[index]))
		return false;
	canReserve_reserveItem(index, item, quantity, std::move(callback));
	return true;
}
SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> Actors::canReserve_unreserveAndReturnBlocksAndCallbacksOnSameDeck(const ActorIndex& index)
{
	DeckId deckId = m_area.m_decks.getForBlock(m_location[index]);
	if(deckId.empty())
		return {};
	auto condition = [&](const BlockIndex& block) { return m_area.m_decks.getForBlock(block) == deckId; };
	return m_canReserve[index]->unreserveAndReturnBlocksAndCallbacksWithCondition(condition);
}

bool Actors::canReserve_hasReservationWith(const ActorIndex& index, Reservable& reservable) const
{
	return m_canReserve[index]->hasReservationWith(reservable);
}
bool Actors::canReserve_canReserveLocation(const ActorIndex& index, const BlockIndex& location, const Facing4& facing) const
{
	Blocks& blocks = m_area.getBlocks();
	FactionId faction = m_faction[index];
	assert(faction.exists());
	for(BlockIndex occupied : Shape::getBlocksOccupiedAt(m_shape[index], blocks, location, facing))
		if(blocks.isReserved(occupied, faction))
			return false;
	return true;
}
bool Actors::canReserve_locationAtEndOfPathIsUnreserved(const ActorIndex& index, const SmallSet<BlockIndex>& path) const
{
	assert(!path.empty());
	Blocks& blocks = m_area.getBlocks();
	Facing4 facing;
	if(path.size() == 1)
		facing = blocks.facingToSetWhenEnteringFrom(path.back(), m_location[index]);
	else
		facing = blocks.facingToSetWhenEnteringFrom(path.back(), *(path.end() - 2));
	return canReserve_canReserveLocation(index, path.back(), facing);
}
CanReserve& Actors::canReserve_get(const ActorIndex& index)
{
	return *m_canReserve[index];
}