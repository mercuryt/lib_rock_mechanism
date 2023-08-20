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
class Faction;
class Reservable;
class CanReserve final
{
	const Faction* m_faction;
	std::unordered_set<Reservable*> m_reservables;
	friend class Reservable;
public:
	CanReserve(const Faction& f) : m_faction(&f) { }
	void clearAll();
	void setFaction(const Faction& faction);
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
		assert(m_reservedCounts.at(canReserve.m_faction) >= m_canReserves.at(&canReserve));
		if(m_reservedCounts.at(canReserve.m_faction) == m_canReserves.at(&canReserve))
			m_reservedCounts.erase(canReserve.m_faction);
		else
			m_reservedCounts.at(canReserve.m_faction) -= m_canReserves.at(&canReserve);
		m_canReserves.erase(&canReserve);
	}
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	~Reservable()
	{
		for(auto& pair : m_canReserves)
			pair.first->m_reservables.erase(this);
	}
	bool isFullyReserved(const Faction& faction) const { return m_reservedCounts.contains(&faction) && m_reservedCounts.at(&faction) == m_maxReservations; }
	std::unordered_map<CanReserve*, uint32_t>& getReservedBy() { return m_canReserves; }
	void reserveFor(CanReserve& canReserve, const uint32_t quantity) 
	{
		assert(!m_reservedCounts.contains(canReserve.m_faction) || m_reservedCounts.at(canReserve.m_faction) + quantity <= m_maxReservations);
		assert(!m_canReserves.contains(&canReserve));
		m_canReserves[&canReserve] = quantity;
		m_reservedCounts[canReserve.m_faction] += quantity;
		assert(!canReserve.m_reservables.contains(this));
		canReserve.m_reservables.insert(this);
	}
	void clearReservationFor(CanReserve& canReserve)
       	{
		assert(m_canReserves.contains(&canReserve));
		assert(m_canReserves.at(&canReserve) >= m_reservedCounts.at(canReserve.m_faction));
		eraseReservationFor(canReserve);
		assert(canReserve.m_reservables.contains(this));
		canReserve.m_reservables.erase(this);
	}
	void setMaxReservations(const uint32_t mr) { m_maxReservations = mr; }
	uint32_t getUnreservedCount(const Faction& faction) const
	{
		return m_maxReservations - m_reservedCounts.at(&faction);
	}
};
inline CanReserve::~CanReserve() { clearAll(); }
inline void CanReserve::clearAll()
{
	for(Reservable* reservable : m_reservables)
		reservable->eraseReservationFor(*this);
	m_reservables.clear();
}
