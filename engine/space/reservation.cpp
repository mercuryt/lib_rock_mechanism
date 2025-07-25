#include "space.h"
#include "../reservable.h"
#include "../dataStructures/rtreeData.hpp"
void Space::reserve(const Point3D& point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	Reservable* reservable = nullptr;
	if(!m_reservables.queryAny(point))
	{
		m_reservables.insert(point, std::make_unique<Reservable>(Quantity::create(1)));
		reservable = m_reservables.queryGetOne(point).get();
	}
	else
		reservable = m_reservables.queryGetOne(point).get();
	reservable->reserveFor(canReserve, Quantity::create(1), std::move(callback));
	canReserve.recordReservedPoint(point, reservable);
}
void Space::unreserve(const Point3D& point, CanReserve& canReserve)
{
	assert(m_reservables.queryGetOne(point) != nullptr);
	auto& reservable = *m_reservables.queryGetOneMutable(point);
	assert(reservable.hasAnyReservationsFor(canReserve));
	reservable.clearReservationFor(canReserve);
	if(!reservable.hasAnyReservations())
		m_reservables.remove(point);
	canReserve.eraseReservedPoint(point);
}
void Space::dishonorAllReservations(const Point3D& point)
{
	assert(m_reservables.queryGetOne(point) != nullptr);
	m_reservables.remove(point);
}
void Space::setReservationDishonorCallback(const Point3D& point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	assert(m_reservables.queryGetOne(point) != nullptr);
	m_reservables.queryGetOne(point)->setDishonorCallbackFor(canReserve, std::move(callback));
}
[[nodiscard]] bool Space::isReserved(const Point3D& point, const FactionId& faction) const
{
	if(m_reservables.queryGetOne(point) == nullptr)
		return false;
	return m_reservables.queryGetOne(point)->hasAnyReservationsWith(faction);
}
Reservable& Space::getReservable(const Point3D& point) { return *m_reservables.queryGetOne(point); }

