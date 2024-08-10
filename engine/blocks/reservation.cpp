#include "blocks.h"
#include "../reservable.h"
void Blocks::reserve(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	Reservable* reservable = nullptr;
	if(m_reservables[index] == nullptr)
	{
		m_reservables[index] = std::make_unique<Reservable>(Quantity::create(1));
		reservable = m_reservables[index].get();
	}
	else
		reservable = m_reservables[index].get();
	reservable->reserveFor(canReserve, Quantity::create(1), std::move(callback));
}
void Blocks::unreserve(BlockIndex index, CanReserve& canReserve)
{
	assert(m_reservables[index] != nullptr);
	m_reservables[index]->clearReservationFor(canReserve);
	if(!m_reservables[index]->hasAnyReservations())
		m_reservables[index] = nullptr;
}
void Blocks::unreserveAll(BlockIndex index)
{
	assert(m_reservables[index] != nullptr);
	m_reservables[index] = nullptr;
}
void Blocks::setReservationDishonorCallback(BlockIndex index, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	assert(m_reservables[index] != nullptr);
	m_reservables[index]->setDishonorCallbackFor(canReserve, std::move(callback));
}
[[nodiscard]] bool Blocks::isReserved(BlockIndex index, const FactionId faction) const
{
	if(m_reservables[index] == nullptr)
		return false;
	return m_reservables[index]->hasAnyReservationsWith(faction);
}
