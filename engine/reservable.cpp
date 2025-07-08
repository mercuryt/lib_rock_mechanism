#include "reservable.h"
#include "simulation/simulation.h"
#include "deserializeDishonorCallbacks.h"
#include "numericTypes/types.h"
#include "space/space.h"
#include <bits/ranges_algo.h>
void CanReserve::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	m_faction = data["faction"];
	for(const Json& reservationData : data["reservations"])
	{
		assert(deserializationMemo.m_reservables.contains(reservationData["reservable"].get<uintptr_t>()));
		Reservable& reservable = *deserializationMemo.m_reservables.at(reservationData["reservable"].get<uintptr_t>());
		std::unique_ptr<DishonorCallback> dishonorCallback = data.contains("dishonorCallback") ?
			deserializeDishonorCallback(data["dishonorCallback"], deserializationMemo, area) : nullptr;
		reservable.reserveFor(*this, reservationData["count"].get<Quantity>(), std::move(dishonorCallback));
	}
}
Json CanReserve::toJson() const
{
	Json data{{"faction", m_faction}, {"reservations", Json::array()}, {"address", reinterpret_cast<uintptr_t>(this)}};
	for(Reservable* reservable : m_reservables)
		data["reservations"].push_back(reservable->jsonReservationFor(const_cast<CanReserve&>(*this)));
	return data;
}
void CanReserve::deleteAllWithoutCallback()
{
	for(Reservable* reservable : m_reservables)
	{
		reservable->eraseReservationFor(*this);
		reservable->m_dishonorCallbacks.maybeErase(this);
	}
	m_reservables.clear();
}
void CanReserve::setFaction(FactionId faction)
{
	for(Reservable* reservable : m_reservables)
		reservable->updateFactionFor(*this, m_faction, faction);
	m_faction = faction;
}
bool CanReserve::translateAndReservePositions(Space& space, SmallMap<Point3D, std::unique_ptr<DishonorCallback>>&& previouslyReserved, const Point3D& previousPivot, const Point3D& newPivot, const Facing4& previousFacing, const Facing4& newFacing)
{
	for(auto& [point, dishonorCallback] : previouslyReserved)
	{
		point = point.translate(previousPivot, newPivot, previousFacing, newFacing);
		if(space.isReserved(point, m_faction))
			return false;
		space.reserve(point, *this, std::move(dishonorCallback));
		m_points.insert(point, &space.getReservable(point));
	}
	return true;
}
void CanReserve::recordReservedPoint(const Point3D& point, Reservable* reservable)
{
	m_points.insert(point, reservable);
}
void CanReserve::eraseReservedPoint(const Point3D& point)
{
	m_points.erase(point);
}
bool CanReserve::hasReservationWith(Reservable& reservable) const { return std::ranges::find(m_reservables, &reservable) != m_reservables.end(); }
bool CanReserve::hasReservations() const
{
	return !m_reservables.empty();
}
CanReserve::~CanReserve() { deleteAllWithoutCallback(); }
void Reservable::eraseReservationFor(CanReserve& canReserve)
{
	assert(m_reservedCounts.contains(canReserve.m_faction));
	assert(m_canReserves.contains(&canReserve));
	assert(m_reservedCounts[canReserve.m_faction] >= m_canReserves[&canReserve]);
	if(m_reservedCounts[canReserve.m_faction] == m_canReserves[&canReserve])
		m_reservedCounts.erase(canReserve.m_faction);
	else
		m_reservedCounts[canReserve.m_faction] -= m_canReserves[&canReserve];
	m_canReserves.erase(&canReserve);
}
std::unique_ptr<DishonorCallback> Reservable::unreserveAndReturnCallback(CanReserve& canReserve)
{
	auto output = std::move(m_dishonorCallbacks[&canReserve]);
	eraseReservationFor(canReserve);
	return output;
}
bool Reservable::isFullyReserved(const FactionId faction) const
{
	return m_reservedCounts.contains(faction) && m_reservedCounts[faction] == m_maxReservations;
}
bool Reservable::hasAnyReservations() const { return !m_canReserves.empty(); }
bool Reservable::hasAnyReservationsWith(const FactionId faction) const { return m_reservedCounts.contains(faction); }
bool Reservable::hasAnyReservationsFor(const CanReserve& canReserve) const { return m_canReserves.contains(&const_cast<CanReserve&>(canReserve)); }
SmallMap<CanReserve*, Quantity>& Reservable::getReservedBy() { return m_canReserves; }
void Reservable::reserveFor(CanReserve& canReserve, const Quantity quantity, std::unique_ptr<DishonorCallback> dishonorCallback)
{
	// No reservations are made for actors with no faction because there is no one to reserve it from.
	if(canReserve.m_faction.empty())
	{
		assert(dishonorCallback == nullptr);
		return;
	}
	assert(getUnreservedCount(canReserve.m_faction) >= quantity);
	m_canReserves.getOrInsert(&canReserve, Quantity::create(0)) += quantity;
	assert(m_canReserves[&canReserve] <= m_maxReservations);
	m_reservedCounts.getOrInsert(canReserve.m_faction, Quantity::create(0)) += quantity;
	if(!canReserve.hasReservationWith(*this))
		canReserve.m_reservables.push_back(this);
	if(dishonorCallback != nullptr)
		m_dishonorCallbacks.insert(&canReserve, std::move(dishonorCallback));
}
void Reservable::clearReservationFor(CanReserve& canReserve, const Quantity quantity)
{
	if(canReserve.m_faction.empty())
		return;
	assert(m_canReserves.contains(&canReserve));
	assert(canReserve.hasReservationWith(*this));
	if(m_canReserves[&canReserve] == quantity)
	{
		eraseReservationFor(canReserve);
		util::removeFromVectorByValueUnordered(canReserve.m_reservables, this);
		m_dishonorCallbacks.maybeErase(&canReserve);
	}
	else
	{
		m_canReserves[&canReserve] -= quantity;
		m_reservedCounts[canReserve.m_faction] -= quantity;
	}
}
void Reservable::clearReservationsFor(const FactionId faction)
{
	std::vector<std::pair<CanReserve*, Quantity>> toErase;
	for(auto pair : m_canReserves)
		if(pair.first->m_faction == faction)
			toErase.push_back(pair);
	for(auto& [canReserve, quantity] : toErase)
		clearReservationFor(*canReserve, quantity);
}
void Reservable::maybeClearReservationFor(CanReserve& canReserve, const Quantity quantity)
{
	if(canReserve.hasReservationWith(*this))
		clearReservationFor(canReserve, quantity);
}
void Reservable::setMaxReservations(const Quantity mr)
{
	assert(mr != 0);
	assert(mr != m_maxReservations);
	if(mr < m_maxReservations)
		for(auto& [faction, quantity] : m_reservedCounts)
		{
			if(quantity > mr)
			{
				// There are no longer enough reservables here to honor the original reservation.
				Quantity delta = quantity - mr;
				m_reservedCounts[faction] -= delta;
				for(auto& [canReserve, canReserveQuantity] : m_canReserves)
					if(canReserve->m_faction == faction)
					{
						Quantity canReserveDelta = std::min(delta, canReserveQuantity);
						canReserveQuantity -= canReserveDelta;
						if(m_dishonorCallbacks.contains(canReserve))
							m_dishonorCallbacks[canReserve]->execute(canReserveQuantity + canReserveDelta, canReserveQuantity);
						delta -= canReserveDelta;
						if(delta == 0)
							break;
					}
			}
		}
	m_maxReservations = mr;
}
void Reservable::updateFactionFor(CanReserve& canReserve, FactionId oldFaction, FactionId newFaction)
{
	assert(m_canReserves.contains(&canReserve));
	if(m_reservedCounts[oldFaction] == m_canReserves[&canReserve])
		m_reservedCounts.erase(oldFaction);
	else
		m_reservedCounts[oldFaction] -= m_canReserves[&canReserve];
	m_reservedCounts[newFaction] += m_canReserves[&canReserve];
	// Erase all reservations if faction is set to null.
	m_canReserves.erase(&canReserve);
}
void Reservable::clearAll()
{
	std::vector<std::tuple<CanReserve*, Quantity, DishonorCallback*>> toErase;
	for(auto pair : m_canReserves)
	{
		DishonorCallback* dishonorCallback = m_dishonorCallbacks.contains(pair.first) ? m_dishonorCallbacks[pair.first].get() : nullptr;
		toErase.emplace_back(pair.first, pair.second, dishonorCallback);
	}
	for(auto& [canReserve, quantity, dishonorCallback] : toErase)
	{
		eraseReservationFor(*canReserve);
		util::removeFromVectorByValueUnordered(canReserve->m_reservables, this);
		if(dishonorCallback != nullptr)
			dishonorCallback->execute(quantity, Quantity::create(0));
	}
	m_dishonorCallbacks.clear();
	assert(m_reservedCounts.empty());
	assert(m_canReserves.empty());
}
void Reservable::updateReservedCount(FactionId faction, Quantity count)
{
	assert(m_reservedCounts.contains(faction));
	assert(count <= m_maxReservations);
	m_reservedCounts[faction] = count;
}
void Reservable::merge(Reservable& reservable)
{
	for(auto [canReserve, quantity] : reservable.m_canReserves)
	{
		m_canReserves[canReserve] += quantity;
		m_reservedCounts[canReserve->m_faction] += quantity;
		assert(m_reservedCounts[canReserve->m_faction] <= m_maxReservations);
		m_dishonorCallbacks[canReserve] = std::move(reservable.m_dishonorCallbacks[canReserve]);
	}
	reservable.m_canReserves.clear();
	reservable.m_dishonorCallbacks.clear();
	m_maxReservations += reservable.m_maxReservations;
}
Quantity Reservable::getUnreservedCount(const FactionId faction) const
{
	if(!m_reservedCounts.contains(faction))
		return m_maxReservations;
	return m_maxReservations - m_reservedCounts[faction];
}
Json Reservable::jsonReservationFor(CanReserve& canReserve) const
{
	Json data({
		{"count", m_canReserves[&canReserve]},
		{"reservable", reinterpret_cast<uintptr_t>(this)}
	});
	if(m_dishonorCallbacks.contains(&canReserve))
		data["dishonorCallback"] = m_dishonorCallbacks[&canReserve]->toJson();
	return data;
}
Reservable::~Reservable() { clearAll(); }
