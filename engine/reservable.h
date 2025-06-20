/*
 * Anything which can be reserved should inherit from Reservable.
 * Anything which can make a reservation should inherit from Reservable::CanReserve.
 */

#pragma once
#include "config.h"
#include "numericTypes/types.h"
#include "dishonorCallback.h"
#include "dataStructures/smallSet.h"
#include "dataStructures/smallMap.h"
#include <algorithm>
#include <sys/types.h>
#include <cassert>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

struct Faction;
class Reservable;
class Area;
class Blocks;
struct DeserializationMemo;
// Reservable holds all the data during runtime, but CanReserve is responsible for (de)serialization.
class CanReserve final
{
	// TODO: Store reservables in two maps, one for blocks and one for ActorOrItem.
	FactionId m_faction;
	// TODO: make this a SmallSet
	std::vector<Reservable*> m_reservables;
	SmallMap<BlockIndex, Reservable*> m_blocks;
	friend class Reservable;
public:
	CanReserve(FactionId f) : m_faction(f) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	Json toJson() const;
	void deleteAllWithoutCallback();
	void setFaction(FactionId faction);
	// Returns true if all new positions could be reserved.
	[[nodiscard]] bool translateAndReservePositions(Blocks& blocks, SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>>&& previouslyReserved, const BlockIndex& previousLocation, const BlockIndex& newLocation, const Facing4& previousFacing, const Facing4& newFacing);
	void recordReservedBlock(const BlockIndex& block, Reservable* reservable);
	void eraseReservedBlock(const BlockIndex& block);
	void deleteAllBlockReservationsWithoutCallback();
	template<typename Condition>
	[[nodiscard]] SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> unreserveAndReturnBlocksAndCallbacksWithCondition(Condition&& condition);
	[[nodiscard]] bool hasReservationWith(Reservable& reservable) const;
	[[nodiscard]] bool hasReservations() const;
	~CanReserve();
	CanReserve(CanReserve& reservable) = delete;
};
// TODO: A specalized reservable for block without count.
class Reservable final
{
	SmallMap<CanReserve*, Quantity> m_canReserves;
	SmallMap<FactionId, Quantity> m_reservedCounts;
	SmallMap<CanReserve*, std::unique_ptr<DishonorCallback>> m_dishonorCallbacks;
	Quantity m_maxReservations = Quantity::create(0);
	void eraseReservationFor(CanReserve& canReserve);
public:
	Reservable(Quantity mr) : m_maxReservations(mr) {}
	void reserveFor(CanReserve& canReserve, Quantity quantity = Quantity::create(1), std::unique_ptr<DishonorCallback> dishonorCallback = nullptr);
	void clearReservationFor(CanReserve& canReserve, Quantity quantity = Quantity::create(1));
	void clearReservationsFor(const FactionId faction);
	void maybeClearReservationFor(CanReserve& canReserve, Quantity quantity = Quantity::create(1));
	void setMaxReservations(const Quantity mr);
	void updateFactionFor(CanReserve& canReserve, FactionId oldFaction, FactionId newFaction);
	void setDishonorCallbackFor(CanReserve& canReserve, std::unique_ptr<DishonorCallback> dishonorCallback) { m_dishonorCallbacks.emplace(&canReserve, std::move(dishonorCallback)); }
	void clearAll();
	void updateReservedCount(FactionId faction, Quantity count);
	void merge(Reservable& reservable);
	[[nodiscard]] std::unique_ptr<DishonorCallback> unreserveAndReturnCallback(CanReserve& canReserve);
	[[nodiscard]] bool isFullyReserved(const FactionId faction) const;
	[[nodiscard]] bool hasAnyReservations() const;
	[[nodiscard]] bool hasAnyReservationsWith(const FactionId faction) const;
	[[nodiscard]] bool hasAnyReservationsFor(const CanReserve& canReserve) const;
	[[nodiscard]] SmallMap<CanReserve*, Quantity>& getReservedBy();
	[[nodiscard]] Quantity getUnreservedCount(const FactionId faction) const;
	[[nodiscard]] Quantity getMaxReservations() const { return m_maxReservations; }
	[[nodiscard]] Json jsonReservationFor(CanReserve& canReserve) const;
	~Reservable();
	Reservable(const Reservable&) = delete;
	friend class CanReserve;
};
