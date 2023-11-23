#include "reservable.h"
void CanReserve::clearAll()
{
	for(Reservable* reservable : m_reservables)
		reservable->eraseReservationFor(*this);
	m_reservables.clear();
}
void CanReserve::setFaction(const Faction* faction)
{
	for(Reservable* reservable : m_reservables)
		reservable->updateFactionFor(*this, m_faction, faction);
	m_faction = faction;
}
bool CanReserve::hasFaction() const { return m_faction != nullptr; }
bool CanReserve::hasReservationWith(Reservable& reservable) const { return m_reservables.contains(&reservable); }
bool CanReserve::hasReservations() const { return !m_reservables.empty(); }
CanReserve::~CanReserve() { clearAll(); }
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
	m_dishonorCallbacks.erase(&canReserve);
}
bool Reservable::isFullyReserved(const Faction* faction) const 
{ 
	if(faction == nullptr)
		return false;
	return m_reservedCounts.contains(faction) && m_reservedCounts.at(faction) == m_maxReservations; 
}
std::unordered_map<CanReserve*, uint32_t>& Reservable::getReservedBy() { return m_canReserves; }
void Reservable::reserveFor(CanReserve& canReserve, const uint32_t quantity, std::function<void(uint32_t oldCount, uint32_t newCount)> dishonorCallback) 
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
		m_dishonorCallbacks[&canReserve] = dishonorCallback;
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
void Reservable::clearReservationsFor(const Faction& faction)
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
							m_dishonorCallbacks.at(canReserve)(canReserveQuantity + canReserveDelta, canReserveQuantity);
						delta -= canReserveDelta;
						if(delta == 0)
							break;
					}
			}
		}
	m_maxReservations = mr; 
}
void Reservable::updateFactionFor(CanReserve& canReserve, const Faction* oldFaction, const Faction* newFaction)
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
	std::vector<std::pair<CanReserve*, uint32_t>> toErase;
	for(auto& pair : m_canReserves)
		toErase.push_back(pair);
	for(auto& [canReserve, callback] : m_dishonorCallbacks)
	{
		assert(m_canReserves.contains(canReserve));
		uint32_t quantity = m_canReserves.at(canReserve);
		callback(quantity, 0);
	}
	for(auto& [canReserve, quantity] : toErase)
		clearReservationFor(*canReserve, quantity);
	assert(m_reservedCounts.empty());
	assert(m_canReserves.empty());
	assert(m_dishonorCallbacks.empty());
}
uint32_t Reservable::getUnreservedCount(const Faction& faction) const
{
	if(!m_reservedCounts.contains(&faction))
		return m_maxReservations;
	return m_maxReservations - m_reservedCounts.at(&faction);
}
Reservable::~Reservable() { clearAll(); }
