/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include "config.h"
#include <algorithm>
#include <sys/types.h>
#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

struct Faction;
class Reservable;
struct DeserializationMemo;
struct DishonorCallback
{
	virtual void execute(uint32_t oldCount, uint32_t newCount) = 0;
	virtual ~DishonorCallback() = default;
	virtual Json toJson() const = 0;
};
class CanReserve final
{
	Faction* m_faction;
	std::unordered_set<Reservable*> m_reservables;
	friend class Reservable;
public:
	CanReserve(Faction* f) : m_faction(f) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void deleteAllWithoutCallback();
	void setFaction(Faction* faction);
	[[nodiscard]] bool factionExists() const;
	[[nodiscard]] bool hasReservationWith(Reservable& reservable) const;
	[[nodiscard]] bool hasReservations() const;
	~CanReserve();
	CanReserve(CanReserve& reservable) = delete;
	CanReserve(CanReserve&& reservable) = delete;
};
// TODO: A specalized reservable for block without count ( to save RAM ).
class Reservable final
{
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	std::unordered_map<Faction*, uint32_t> m_reservedCounts;
	std::unordered_map<CanReserve*, std::unique_ptr<DishonorCallback>> m_dishonorCallbacks;
	uint32_t m_maxReservations = 0;
	void eraseReservationFor(CanReserve& canReserve);
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	void reserveFor(CanReserve& canReserve, const uint32_t quantity = 1u, std::unique_ptr<DishonorCallback> dishonorCallback = nullptr); 
	void clearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void clearReservationsFor(Faction& faction);
	void maybeClearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void setMaxReservations(const uint32_t mr);
	void updateFactionFor(CanReserve& canReserve, Faction* oldFaction, Faction* newFaction);
	void setDishonorCallbackFor(CanReserve& canReserve, std::unique_ptr<DishonorCallback> dishonorCallback) { m_dishonorCallbacks[&canReserve] = std::move(dishonorCallback); }
	void clearAll();
	void updateReservedCount(Faction& faction, uint32_t count);
	void merge(Reservable& reservable);
	[[nodiscard]] bool isFullyReserved(Faction* faction) const;
	[[nodiscard]] bool hasAnyReservations() const;
	[[nodiscard]] bool hasAnyReservationsWith(const Faction& faction) const;
	[[nodiscard]] std::unordered_map<CanReserve*, uint32_t>& getReservedBy();
	[[nodiscard]] uint32_t getUnreservedCount(const Faction& faction) const;
	[[nodiscard]] uint32_t getMaxReservations() const { return m_maxReservations; }
	[[nodiscard]] Json jsonReservationFor(CanReserve& canReserve) const;
	~Reservable();
	Reservable(const Reservable&) { assert(false); }
	Reservable(Reservable&&) { assert(false); }
	friend class CanReserve;
};
