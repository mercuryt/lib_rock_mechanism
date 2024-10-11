#include "actors.h"
#include "../area.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
void Actors::canReserve_clearAll(ActorIndex index)
{
	m_canReserve[index]->deleteAllWithoutCallback();
}
void Actors::canReserve_setFaction(ActorIndex index, FactionId faction)
{
	m_canReserve[index]->setFaction(faction);
}
void Actors::canReserve_reserveLocation(ActorIndex index, BlockIndex block, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, index.toReference(m_area));
	m_area.getBlocks().reserve(block, canReserve_get(index), std::move(callback));
}
void Actors::canReserve_reserveItem(ActorIndex index, ItemIndex item, Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, index.toReference(m_area));
	m_area.getItems().reservable_reserve(item, canReserve_get(index), quantity, std::move(callback));
}
bool Actors::canReserve_tryToReserveLocation(ActorIndex index, BlockIndex block, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, index.toReference(m_area));
	if(m_area.getBlocks().isReserved(block, getFactionId(index)))
		return false;
	canReserve_reserveLocation(index, block, std::move(callback));
	return true;
}
bool Actors::canReserve_tryToReserveItem(ActorIndex index, ItemIndex item, Quantity quantity, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, index.toReference(m_area));
	if(m_area.getItems().reservable_isFullyReserved(item, m_faction[index]))
		return false;
	canReserve_reserveItem(index, item, quantity, std::move(callback));
	return true;
}
bool Actors::canReserve_hasReservationWith(ActorIndex index, Reservable& reservable) const
{
	return m_canReserve[index]->hasReservationWith(reservable);
}
bool Actors::canReserve_canReserveLocation(const ActorIndex& index, const BlockIndex& location, const Facing& facing) const
{
	Blocks& blocks = m_area.getBlocks();
	FactionId faction = m_faction[index];
	assert(faction.exists());
	for(BlockIndex occupied : Shape::getBlocksOccupiedAt(m_shape[index], blocks, location, facing))
		if(blocks.isReserved(occupied, faction))
			return false;
	return true;
}
CanReserve& Actors::canReserve_get(ActorIndex index)
{
	return *m_canReserve[index];
}
