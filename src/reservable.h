/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include <sys/types.h>
#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <vector>
#include <functional>

using DishonorCallback = std::function<void(uint32_t oldCount, uint32_t newCount)>;
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
	[[nodiscard]] bool hasFaction() const;
	[[nodiscard]] bool hasReservationWith(Reservable& reservable) const;
	[[nodiscard]] bool hasReservations() const;
	~CanReserve();
};
// TODO: A specalized reservable for block without count ( to save RAM ).
class Reservable final
{
	friend class CanReserve;
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	uint32_t m_maxReservations;
	std::unordered_map<const Faction*, uint32_t> m_reservedCounts;
	std::unordered_map<CanReserve*, DishonorCallback> m_dishonorCallbacks;
	void eraseReservationFor(CanReserve& canReserve);
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	bool isFullyReserved(const Faction* faction) const;
	std::unordered_map<CanReserve*, uint32_t>& getReservedBy();
	void reserveFor(CanReserve& canReserve, const uint32_t quantity = 1u, DishonorCallback dishonorCallback = nullptr); 
	void clearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void clearReservationsFor(const Faction& faction);
	void maybeClearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void setMaxReservations(const uint32_t mr);
	void updateFactionFor(CanReserve& canReserve, const Faction* oldFaction, const Faction* newFaction);
	void setDishonorCallbackFor(CanReserve& canReserve, DishonorCallback dishonorCallback) { m_dishonorCallbacks[&canReserve] = dishonorCallback; }
	void clearAll();
	uint32_t getUnreservedCount(const Faction& faction) const;
	~Reservable();
};
