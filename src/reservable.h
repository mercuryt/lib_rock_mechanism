/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include <unordered_set>

class CanReserve final
{
	friend class Reserveable;
	std::unordered_set<Reserveable*> m_reservables;
	~CanReserve();
};
class Reserveable final
{
	friend class CanReserve;
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	uint32_t m_maxReservations;
	uint32_t m_reservedCount;
	Reserveable(uint32_t mr) : m_maxReservations(r) {}
	~Reserveable()
	{
		for(CanReserve* canReserve : m_canReserves)
			canReserve->m_reservables.remove(this);
	}
public:
	bool isFullyReserved() const { return m_reservedCount == m_maxReservations; }
	std::unordered_map<CanReserve*, uint32_t>& getReservedBy() { return m_canReserves; }
	void reserveFor(CanReserve& canReserve, uint32_t quantity) 
	{
		assert(m_reservedCount + quantity <= m_maxReservations);
		assert(!m_canReserves.contains(&canReserve));
		m_canReserves[canReserve] = quantity;
		m_reservedCount += quantity;
		assert(!canReserve.m_reservables.contains(this));
		canReserve.m_reservables.insert(this);
	}
	void clearReservationFor(CanReserve& canReserve)
       	{
		assert(m_canReserves.contains(&canReserve));
		assert(m_canReserves[&canReserve] >= m_reservedCount);
		m_reservedCount -= m_canReserves[&canReserve];
		m_canReserves.remove(&canReserve);
		assert(canReserve.m_reservables.contains(this));
		canReserve.m_reservables.remove(this);
	}
	void setMaxReservations(uint32_t mr) { m_maxReservations = mr; }
};
CanReserve::~CanReserve()
{
	for(Reserveable* reservable : m_reservables)
		reservable.m_reservations.remove(this);
}
