/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include <unordered_set>
class Reservable;
class CanReserve final
{
	friend class Reservable;
	std::unordered_set<Reservable*> m_reservables;
	~CanReserve();
};
class Reservable final
{
	friend class CanReserve;
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	uint32_t m_maxReservations;
	uint32_t m_reservedCount;
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	~Reservable()
	{
		for(auto& pair : m_canReserves)
			pair.first->m_reservables.erase(this);
	}
	bool isFullyReserved() const { return m_reservedCount == m_maxReservations; }
	std::unordered_map<CanReserve*, uint32_t>& getReservedBy() { return m_canReserves; }
	void reserveFor(CanReserve& canReserve, uint32_t quantity) 
	{
		assert(m_reservedCount + quantity <= m_maxReservations);
		assert(!m_canReserves.contains(&canReserve));
		m_canReserves[&canReserve] = quantity;
		m_reservedCount += quantity;
		assert(!canReserve.m_reservables.contains(this));
		canReserve.m_reservables.insert(this);
	}
	void clearReservationFor(CanReserve& canReserve)
       	{
		assert(m_canReserves.contains(&canReserve));
		assert(m_canReserves[&canReserve] >= m_reservedCount);
		m_reservedCount -= m_canReserves[&canReserve];
		m_canReserves.erase(&canReserve);
		assert(canReserve.m_reservables.contains(this));
		canReserve.m_reservables.erase(this);
	}
	void setMaxReservations(uint32_t mr) { m_maxReservations = mr; }
	uint32_t getUnreservedCount() const
	{
		return m_maxReservations - m_reservedCount;
	}
};
inline CanReserve::~CanReserve()
{
	for(Reservable* reservable : m_reservables)
		reservable->m_canReserves.erase(this);
}
