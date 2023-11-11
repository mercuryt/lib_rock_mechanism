/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <vector>
class Faction;
class Reservable;
class CanReserve final
{
	const Faction* m_faction;
	std::unordered_set<Reservable*> m_reservables;
	friend class Reservable;
public:
	CanReserve(const Faction* f) : m_faction(f) { }
	void clearAll();
	void setFaction(const Faction* faction);
	[[nodiscard]] bool hasFaction() const { return m_faction != nullptr; }
	[[nodiscard]] bool hasReservationWith(Reservable& reservable) const { return m_reservables.contains(&reservable); }
	[[nodiscard]] bool hasReservations() const { return !m_reservables.empty(); }
	~CanReserve();
};
class Reservable final
{
	friend class CanReserve;
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	uint32_t m_maxReservations;
	std::unordered_map<const Faction*, uint32_t> m_reservedCounts;
	void eraseReservationFor(CanReserve& canReserve)
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
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	bool isFullyReserved(const Faction* faction) const 
	{ 
		if(faction == nullptr)
			return false;
		return m_reservedCounts.contains(faction) && m_reservedCounts.at(faction) == m_maxReservations; 
	}
	std::unordered_map<CanReserve*, uint32_t>& getReservedBy() { return m_canReserves; }
	void reserveFor(CanReserve& canReserve, const uint32_t quantity = 1u) 
	{
		// No reservations are made for actors with no faction because there is no one to reserve it from.
		if(canReserve.m_faction == nullptr)
			return;
		assert(getUnreservedCount(*canReserve.m_faction) >= quantity);
		m_canReserves[&canReserve] += quantity;
		assert(m_canReserves[&canReserve] <= m_maxReservations);
		m_reservedCounts[canReserve.m_faction] += quantity;
		if(!canReserve.m_reservables.contains(this))
			canReserve.m_reservables.insert(this);
	}
	void maybeReserveFor(CanReserve& canReserve, const uint32_t quantity = 1u)
	{
		if(canReserve.m_faction == nullptr)
			return;
		if(!m_canReserves.contains(&canReserve))
			reserveFor(canReserve, quantity);
	}
	void clearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u)
       	{
		if(canReserve.m_faction == nullptr)
			return;
		assert(m_canReserves.contains(&canReserve));
		assert(canReserve.m_reservables.contains(this));
		if(m_canReserves.at(&canReserve) == quantity)
		{
			eraseReservationFor(canReserve);
			canReserve.m_reservables.erase(this);
		}
		else
		{
			m_canReserves.at(&canReserve) -= quantity;
			m_reservedCounts.at(canReserve.m_faction) -= quantity;
		}
	}
	void clearReservationsFor(const Faction& faction)
	{
		std::vector<std::pair<CanReserve*, uint32_t>> toErase;
		for(auto& pair : m_canReserves)
			if(pair.first->m_faction == &faction)
				toErase.push_back(pair);
		for(auto& [canReserve, quantity] : toErase)
			clearReservationFor(*canReserve, quantity);
	}
	void maybeClearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u)
	{
		if(canReserve.m_reservables.contains(this))
			clearReservationFor(canReserve, quantity);
	}
	//TODO: Notify reservations which cannot be honored.
	void setMaxReservations(const uint32_t mr) { m_maxReservations = mr; }
	void updateFactionFor(CanReserve& canReserve, const Faction* oldFaction, const Faction* newFaction)
	{
		assert(m_canReserves.contains(&canReserve));
		if(oldFaction != nullptr)
			m_reservedCounts[oldFaction] -= m_canReserves[&canReserve];
		if(newFaction != nullptr)
			m_reservedCounts[newFaction] += m_canReserves[&canReserve];
		else
			// Erase all reservations if faction is set to null.
			m_canReserves.erase(&canReserve);
	}
	void clearAll()
	{
		std::vector<std::pair<CanReserve*, uint32_t>> toErase;
		for(auto& pair : m_canReserves)
			toErase.push_back(pair);
		for(auto& [canReserve, quantity] : toErase)
			clearReservationFor(*canReserve, quantity);
		assert(m_reservedCounts.empty());
		assert(m_canReserves.empty());
	}
	uint32_t getUnreservedCount(const Faction& faction) const
	{
		if(!m_reservedCounts.contains(&faction))
			return m_maxReservations;
		return m_maxReservations - m_reservedCounts.at(&faction);
	}
	~Reservable() { clearAll(); }
};
inline void CanReserve::clearAll()
{
	for(Reservable* reservable : m_reservables)
		reservable->eraseReservationFor(*this);
	m_reservables.clear();
}
inline void CanReserve::setFaction(const Faction* faction)
{
	for(Reservable* reservable : m_reservables)
		reservable->updateFactionFor(*this, m_faction, faction);
	m_faction = faction;
}
inline CanReserve::~CanReserve() { clearAll(); }
