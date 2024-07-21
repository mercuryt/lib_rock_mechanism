/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
//TODO: Use vector instead of unordered set.
#include "config.h"
#include "types.h"
#include "dishonorCallback.h"
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
// Reservable holds all the data during runtime, but CanReserve is responsible for (de)serialization.
class CanReserve final
{
	FactionId m_faction;
	std::unordered_set<Reservable*> m_reservables;
	friend class Reservable;
public:
	CanReserve(FactionId f) : m_faction(f) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void deleteAllWithoutCallback();
	void setFaction(FactionId faction);
	[[nodiscard]] bool hasReservationWith(Reservable& reservable) const;
	[[nodiscard]] bool hasReservations() const;
	~CanReserve();
	CanReserve(CanReserve& reservable) = delete;
};
// TODO: A specalized reservable for block without count ( to save RAM ).
class Reservable final
{
	std::unordered_map<CanReserve*, uint32_t> m_canReserves;
	std::unordered_map<FactionId, uint32_t> m_reservedCounts;
	std::unordered_map<CanReserve*, std::unique_ptr<DishonorCallback>> m_dishonorCallbacks;
	uint32_t m_maxReservations = 0;
	void eraseReservationFor(CanReserve& canReserve);
public:
	Reservable(uint32_t mr) : m_maxReservations(mr) {}
	void reserveFor(CanReserve& canReserve, const uint32_t quantity = 1u, std::unique_ptr<DishonorCallback> dishonorCallback = nullptr); 
	void clearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void clearReservationsFor(const FactionId faction);
	void maybeClearReservationFor(CanReserve& canReserve, const uint32_t quantity = 1u);
	void setMaxReservations(const uint32_t mr);
	void updateFactionFor(CanReserve& canReserve, FactionId oldFaction, FactionId newFaction);
	void setDishonorCallbackFor(CanReserve& canReserve, std::unique_ptr<DishonorCallback> dishonorCallback) { m_dishonorCallbacks[&canReserve] = std::move(dishonorCallback); }
	void clearAll();
	void updateReservedCount(FactionId faction, uint32_t count);
	void merge(Reservable& reservable);
	[[nodiscard]] bool isFullyReserved(const FactionId faction) const;
	[[nodiscard]] bool hasAnyReservations() const;
	[[nodiscard]] bool hasAnyReservationsWith(const FactionId faction) const;
	[[nodiscard]] std::unordered_map<CanReserve*, uint32_t>& getReservedBy();
	[[nodiscard]] uint32_t getUnreservedCount(const FactionId faction) const;
	[[nodiscard]] uint32_t getMaxReservations() const { return m_maxReservations; }
	[[nodiscard]] Json jsonReservationFor(CanReserve& canReserve) const;
	~Reservable();
	Reservable(const Reservable&) = delete;
	friend class CanReserve;
};
