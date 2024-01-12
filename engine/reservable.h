/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include "config.h"
#include "deserializationMemo.h"
#include <algorithm>
#include <sys/types.h>
#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

class Faction;
class Reservable;
struct DishonorCallback
{
	virtual void execute(uint32_t oldCount, uint32_t newCount) = 0;
	virtual ~DishonorCallback() = default;
	virtual Json toJson() const = 0;
};
class CanReserve final
{
	const Faction* m_faction;
	std::unordered_set<Reservable*> m_reservables;
	friend class Reservable;
public:
	CanReserve(const Faction* f) : m_faction(f) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void clearAll();
	void setFaction(const Faction* faction);
	void deleteAllWithoutCallback() { m_reservables.clear(); }
	[[nodiscard]] bool hasFaction() const;
	[[nodiscard]] bool hasReservationWith(Reservable& reservable) const;
	[[nodiscard]] bool hasReservations() const;
	~CanReserve();
	CanReserve(CanReserve& reservable) = delete;
	CanReserve(CanReserve&& reservable) = delete;
};
// TODO: A specalized reservable for block without count ( to save RAM ).
class Reservable final
{
	friend class CanReserve;
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	uint32_t m_maxReservations;
	std::unordered_map<const Faction*, uint32_t> m_reservedCounts;
	std::unordered_map<CanReserve*, std::unique_ptr<DishonorCallback>> m_dishonorCallbacks;
	void eraseReservationFor(CanReserve& canReserve);
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	bool isFullyReserved(const Faction* faction) const;
	std::unordered_map<CanReserve*, uint32_t>& getReservedBy();
	void reserveFor(CanReserve& canReserve, const uint32_t quantity = 1u, std::unique_ptr<DishonorCallback> dishonorCallback = nullptr); 
	void clearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void clearReservationsFor(const Faction& faction);
	void maybeClearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void setMaxReservations(const uint32_t mr);
	void updateFactionFor(CanReserve& canReserve, const Faction* oldFaction, const Faction* newFaction);
	void setDishonorCallbackFor(CanReserve& canReserve, std::unique_ptr<DishonorCallback> dishonorCallback) { m_dishonorCallbacks[&canReserve] = std::move(dishonorCallback); }
	void clearAll();
	uint32_t getUnreservedCount(const Faction& faction) const;
	Json jsonReservationFor(CanReserve& canReserve) const;
	~Reservable();
	Reservable(Reservable& reservable) = delete;
	Reservable(Reservable&& reservable) = delete;
};
