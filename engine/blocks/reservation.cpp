#include "blocks.h"
#include "../reservable.h"
void Blocks::reserve(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	Reservable* reservable = nullptr;
	std::unordered_map<BlockIndex, Reservable>::iterator found = m_reservables.find(index);
	if(found == m_reservables.end())
	{
		auto pair = m_reservables.try_emplace(index, 1);
		assert(pair.second);
		reservable = &pair.first->second;
	}
	else
		reservable = &found->second;
	reservable->reserveFor(canReserve, 1, std::move(callback));
}
void Blocks::unreserve(BlockIndex index, CanReserve& canReserve)
{
	auto iter = m_reservables.find(index);
	assert(iter != m_reservables.end());
	iter->second.clearReservationFor(canReserve);
}
void Blocks::unreserveAll(BlockIndex index)
{
	auto iter = m_reservables.find(index);
	assert(iter != m_reservables.end());
	iter->second.clearAll();
}
void Blocks::setReservationDishonorCallback(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	auto iter = m_reservables.find(index);
	assert(iter != m_reservables.end());
	iter->second.setDishonorCallbackFor(canReserve, std::move(callback));
}
[[nodiscard]] bool Blocks::isReserved(BlockIndex index, const Faction& faction) const
{
	std::unordered_map<BlockIndex, Reservable>::const_iterator found = m_reservables.find(index);
	if(found == m_reservables.end())
		return false;
	return found->second.hasAnyReservationsWith(faction);
}
