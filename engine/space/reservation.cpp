#include "space.h"
#include "../reservable.h"
void Space::reserve(const Point3D& point, CanReserve& canReserve, std::unique_ptr<DishonorCallback> callback)
{
	const auto action = [&](std::unique_ptr<Reservable>& reservable)
	{
		if(reservable == nullptr)
			reservable = std::make_unique<Reservable>(Quantity::create(1));
		reservable->reserveFor(canReserve, {1}, std::move(callback));
	};
	m_reservables.updateOrInsertOne(point, action);
}
void Space::unreserve(const Point3D& point, CanReserve& canReserve)
{
	assert(m_reservables.queryAny(point));
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
bool Space::isReserved(const Point3D& point, const FactionId& faction) const
{
	return isReservedAny(Cuboid(point, point), faction);
}
bool Space::isReservedAny(const Cuboid& cuboid, const FactionId& faction) const
{
	const auto condition = [&](const std::unique_ptr<Reservable>& reservable){ return reservable->hasAnyReservationsWith(faction); };
	return m_reservables.queryAnyWithCondition(cuboid, condition);
}
bool Space::isReservedAny(const CuboidSet& cuboids, const FactionId& faction) const
{
	const auto condition = [&](const std::unique_ptr<Reservable>& reservable){ return reservable->hasAnyReservationsWith(faction); };
	return m_reservables.batchQueryAnyWithCondition(cuboids, condition);
}
Reservable& Space::getReservable(const Point3D& point) { return *m_reservables.queryGetOne(point); }

