/*
 * A shared base class for Actor and Item.
 */
#pragma once

#include "leadAndFollow.h"
#include "onDestroy.h"
#include "reservable.h"
#include "types.h"

#include <unordered_set>

class Block;
class BlockHasShapes;
class Actor;
class Item;
struct Faction;
class CanReserve;
class Simulation;
struct MoveType;
struct ItemType;
struct Shape;
struct DeserializationMemo;

class HasShape
{
	Simulation& m_simulation;
protected:
	HasShape(Simulation& simulation, const Shape& shape, bool isStatic, Facing f = 0u, uint32_t maxReservations = 1u, Faction* faction = nullptr) : m_simulation(simulation), m_static(isStatic), m_faction(faction), m_shape(&shape), m_location(nullptr), m_facing(f), m_canLead(*this), m_canFollow(*this), m_reservable(maxReservations), m_isUnderground(false)  {}
	HasShape(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	bool m_static = false;
	Faction* m_faction = nullptr;
public:
	const Shape* m_shape = nullptr;
	Block* m_location = nullptr;
	Facing m_facing= 0; 
	std::unordered_set<Block*> m_blocks;
	//TODO: Adjacent blocks cache.
	CanLead m_canLead;
	CanFollow m_canFollow;
	OnDestroy m_onDestroy;
	Reservable m_reservable;
	bool m_isUnderground = false;

	void setShape(const Shape& shape);
	void setStatic(bool isTrue);
	void reserveOccupied(CanReserve& canReserve);
	// May return nullptr.
	[[nodiscard]] Faction* getFaction() const { return m_faction; }
	[[nodiscard]] bool isAdjacentTo(const HasShape& other) const;
	[[nodiscard]] bool isAdjacentTo(Block& block) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlock(std::function<bool(const Block&)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentBlock(std::function<bool(const Block&)> predicate) const;
	[[nodiscard]] std::unordered_set<Block*> getAdjacentBlocks();
	[[nodiscard]] std::unordered_set<HasShape*> getAdjacentHasShapes();
	[[nodiscard]] std::vector<Block*> getAdjacentAtLocationWithFacing(const Block& block, Facing facing);
	[[nodiscard]] std::vector<Block*> getBlocksWhichWouldBeOccupiedAtLocationAndFacing(Block& location, Facing facing);
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing, const Faction& faction) const;
	[[nodiscard]] bool allOccupiedBlocksAreReservable(const Faction& faction) const;
	[[nodiscard]] bool isAdjacentToAt(const Block& location, Facing facing, const HasShape& hasShape) const;
	[[nodiscard]] DistanceInBlocks distanceTo(const HasShape& other) const;
	[[nodiscard]] Block* getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Block&)>& predicate);
	[[nodiscard]] Block* getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Block&)>& predicate);
	[[nodiscard]] Block* getBlockWhichIsAdjacentWithPredicate(std::function<bool(const Block&)>& predicate);
	[[nodiscard]] Block* getBlockWhichIsOccupiedWithPredicate(std::function<bool(const Block&)>& predicate);
	[[nodiscard]] Item* getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Item&)>& predicate);
	[[nodiscard]] Item* getItemWhichIsAdjacentWithPredicate(std::function<bool(const Item&)>& predicate);
	[[nodiscard]] Simulation& getSimulation() { return m_simulation; }
	[[nodiscard]] EventSchedule& getEventSchedule();
	virtual bool isItem() const = 0;
	virtual bool isActor() const = 0;
	virtual bool isGeneric() const = 0;
	virtual Mass getMass() const = 0;
	virtual Volume getVolume() const = 0;
	virtual void setLocation(Block& block) = 0;
	virtual void exit() = 0;
	virtual const MoveType& getMoveType() const = 0;
	virtual Mass singleUnitMass() const = 0;
	virtual Quantity getQuantity() const { return 1; }
	friend class BlockHasShapes;
	// For debugging.
	virtual void log() const;
};
