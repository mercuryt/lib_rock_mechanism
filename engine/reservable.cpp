#include "reservable.h"
#include "simulation.h"
#include "deserializeDishonorCallbacks.h"
#include <bits/ranges_algo.h>
void CanReserve::load(const Json& data, DeserializationMemo& deserializationMemo)
{ 
	m_faction = &deserializationMemo.faction(data["faction"].get<std::wstring>());
	for(const Json& reservationData : data["reservations"])
	{
		assert(deserializationMemo.m_reservables.contains(reservationData["reservable"].get<uintptr_t>()));
		Reservable& reservable = *deserializationMemo.m_reservables.at(reservationData["reservable"].get<uintptr_t>());
		std::unique_ptr<DishonorCallback> dishonorCallback = data.contains("dishonorCallback") ?
			deserializeDishonorCallback(data["dishonorCallback"], deserializationMemo) : nullptr;
		reservable.reserveFor(*this, reservationData["count"].get<uint32_t>(), std::move(dishonorCallback));
	}
}
Json CanReserve::toJson() const 
{
	Json data{{"faction", m_faction->name}, {"reservations", Json::array()}, {"address", reinterpret_cast<uintptr_t>(this)}};
	for(Reservable* reservable : m_reservables)
		data["reservations"].push_back(reservable->jsonReservationFor(const_cast<CanReserve&>(*this)));
	return data;
}
void CanReserve::deleteAllWithoutCallback()
{
	for(Reservable* reservable : m_reservables)
	{
		reservable->eraseReservationFor(*this);
		reservable->m_dishonorCallbacks.erase(this);
	}
	m_reservables.clear();
}
void CanReserve::setFaction(Faction* faction)
{
	for(Reservable* reservable : m_reservables)
		reservable->updateFactionFor(*this, m_faction, faction);
	m_faction = faction;
}
bool CanReserve::hasFaction() const { return m_faction != nullptr; }
bool CanReserve::hasReservationWith(Reservable& reservable) const { return m_reservables.contains(&reservable); }
bool CanReserve::hasReservations() const { return !m_reservables.empty(); }
CanReserve::~CanReserve() { deleteAllWithoutCallback(); }
void Reservable::eraseReservationFor(CanReserve& canReserve)
{
	assert(m_reservedCounts.contains(canReserve.m_faction));
	assert(m_canReserves.contains(&canReserve));
	assert(m_reservedCounts.at(canReserve.m_faction) >= m_canReserves.at(&canReserve));
	if(m_reservedCounts.at(canReserve.m_faction) == m_canReserves.at(&canReserve))
		m_reservedCounts.erase(canReserve.m_faction);
	else
		m_reservedCounts.at(canReserve.m_faction) -= m_canReserves.at(&canReserve);
	m_canReserves.erase(&canReserve);
}
bool Reservable::isFullyReserved(Faction* faction) const 
{ 
	if(faction == nullptr)
		return false;
	return m_reservedCounts.contains(faction) && m_reservedCounts.at(faction) == m_maxReservations; 
}
bool Reservable::hasAnyReservations() const { return !m_canReserves.empty(); }
bool Reservable::hasAnyReservationsWith(Faction& faction) const { return m_reservedCounts.contains(&faction); }
std::unordered_map<CanReserve*, uint32_t>& Reservable::getReservedBy() { return m_canReserves; }
void Reservable::reserveFor(CanReserve& canReserve, const uint32_t quantity, std::unique_ptr<DishonorCallback> dishonorCallback) 
{
	// No reservations are made for actors with no faction because there is no one to reserve it from.
	if(canReserve.m_faction == nullptr)
	{
		assert(dishonorCallback == nullptr);
		return;
	}
	assert(getUnreservedCount(*canReserve.m_faction) >= quantity);
	m_canReserves[&canReserve] += quantity;
	assert(m_canReserves[&canReserve] <= m_maxReservations);
	m_reservedCounts[canReserve.m_faction] += quantity;
	if(!canReserve.m_reservables.contains(this))
		canReserve.m_reservables.insert(this);
	if(dishonorCallback != nullptr)
		m_dishonorCallbacks[&canReserve] = std::move(dishonorCallback);
}
void Reservable::clearReservationFor(CanReserve& canReserve, const uint32_t quantity)
{
	if(canReserve.m_faction == nullptr)
		return;
	assert(m_canReserves.contains(&canReserve));
	assert(canReserve.m_reservables.contains(this));
	if(m_canReserves.at(&canReserve) == quantity)
	{
		eraseReservationFor(canReserve);
		canReserve.m_reservables.erase(this);
		m_dishonorCallbacks.erase(&canReserve);
	}
	else
	{
		m_canReserves.at(&canReserve) -= quantity;
		m_reservedCounts.at(canReserve.m_faction) -= quantity;
	}
}
void Reservable::clearReservationsFor(Faction& faction)
{
	std::vector<std::pair<CanReserve*, uint32_t>> toErase;
	for(auto& pair : m_canReserves)
		if(pair.first->m_faction == &faction)
			toErase.push_back(pair);
	for(auto& [canReserve, quantity] : toErase)
		clearReservationFor(*canReserve, quantity);
}
void Reservable::maybeClearReservationFor(CanReserve& canReserve, const uint32_t quantity)
{
	if(canReserve.m_reservables.contains(this))
		clearReservationFor(canReserve, quantity);
}
void Reservable::setMaxReservations(const uint32_t mr) 
{
	assert(mr != 0);
	assert(mr != m_maxReservations);
	if(mr < m_maxReservations)
		for(auto& [faction, quantity] : m_reservedCounts)
		{
			if(quantity > mr)
			{
				// There are no longer enough reservables here to honor the original reservation.
				uint32_t delta = quantity - mr;
				m_reservedCounts.at(faction) -= delta;
				for(auto& [canReserve, canReserveQuantity] : m_canReserves)
					if(canReserve->m_faction == faction)
					{
						uint32_t canReserveDelta = std::min(delta, canReserveQuantity);
						canReserveQuantity -= canReserveDelta;
						if(m_dishonorCallbacks.contains(canReserve))
							m_dishonorCallbacks.at(canReserve)->execute(canReserveQuantity + canReserveDelta, canReserveQuantity);
						delta -= canReserveDelta;
						if(delta == 0)
							break;
					}
			}
		}
	m_maxReservations = mr; 
}
void Reservable::updateFactionFor(CanReserve& canReserve, Faction* oldFaction, Faction* newFaction)
{
	assert(m_canReserves.contains(&canReserve));
	if(oldFaction != nullptr)
	{
		if(m_reservedCounts.at(oldFaction) == m_canReserves[&canReserve])
			m_reservedCounts.erase(oldFaction);
		else
			m_reservedCounts[oldFaction] -= m_canReserves[&canReserve];
	}
	if(newFaction != nullptr)
		m_reservedCounts[newFaction] += m_canReserves[&canReserve];
	else
		// Erase all reservations if faction is set to null.
		m_canReserves.erase(&canReserve);
}
void Reservable::clearAll()
{
	std::vector<std::tuple<CanReserve*, uint32_t, DishonorCallback*>> toErase;
	for(auto& pair : m_canReserves)
	{
		DishonorCallback* dishonorCallback = m_dishonorCallbacks.contains(pair.first) ? m_dishonorCallbacks.at(pair.first).get() : nullptr;
		toErase.emplace_back(pair.first, pair.second, dishonorCallback);
	}
	for(auto& [canReserve, quantity, dishonorCallback] : toErase)
	{
		eraseReservationFor(*canReserve);
		canReserve->m_reservables.erase(this);
		if(dishonorCallback != nullptr)
			dishonorCallback->execute(quantity, 0);
	}
	m_dishonorCallbacks.clear();
	assert(m_reservedCounts.empty());
	assert(m_canReserves.empty());
}
void Reservable::updateReservedCount(Faction& faction, uint32_t count)
{
	assert(m_reservedCounts.contains(&faction));
	assert(count <= m_maxReservations);
	m_reservedCounts[&faction] = count;
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
}
uint32_t Reservable::getUnreservedCount(Faction& faction) const
{
	if(!m_reservedCounts.contains(&faction))
		return m_maxReservations;
	return m_maxReservations - m_reservedCounts.at(&faction);
}
Json Reservable::jsonReservationFor(CanReserve& canReserve) const
{
	Json data({
		{"count", m_canReserves.at(&canReserve)},
		{"reservable", reinterpret_cast<uintptr_t>(this)}
	});
	if(m_dishonorCallbacks.contains(&canReserve))
		data["dishonorCallback"] = m_dishonorCallbacks.at(&canReserve)->toJson();
	return data;
}
Reservable::~Reservable() { clearAll(); }
