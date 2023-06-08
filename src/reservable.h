/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include <unordered_set>

class CanReserve
{
	friend class Reserveable;
	std::unordered_set<Reserveable*> m_reservables;
	~CanReserve();
};
class Reserveable
{
	friend class CanReserve;
	std::unordered_set<CanReserve*> m_canReserves;
	size_t m_maxReservations;
	Reserveable(size_t mr) : m_maxReservations(r) {}
	virtual ~Reserveable()
	{
		for(CanReserve* canReserve : m_canReserves)
			canReserve->m_reservables.remove(this);
	}
public:
	bool isFullyReserved() const { return m_canReserves.size() == m_maxReservations; }
	std::unordered_set<CanReserve*>& getReservedBy() { return m_canReserves; }
	void reserveFor(CanReserve& canReserve) 
	{
		assert(m_canReserves.size() < m_maxReservations);
		assert(!m_canReserves.contains(&canReserve));
		m_canReserves.insert(&canReserve);
		assert(!canReserve.m_reservables.contains(this));
		canReserve.m_reservables.insert(this);
	}
	void clearReservationFor(CanReserve& canReserve)
       	{
		assert(m_canReserves.contains(&canReserve));
		m_canReserves.remove(&canReserve);
		assert(canReserve.m_reservables.contains(this));
		canReserve.m_reservables.remove(this);
	}
};
CanReserve::~CanReserve()
{
	for(Reserveable* reservable : m_reservables)
		reservable.m_reservations.remove(this);
}
