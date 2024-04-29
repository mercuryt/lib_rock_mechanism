/*
 * A shared base class for Actor and Item.
 */
#pragma once

#include "shape.h"
#include "leadAndFollow.h"
#include "onDestroy.h"
#include "reservable.h"
#include "types.h"

#include <unordered_set>

class Block;
struct MoveType;
class BlockHasShapes;
class Actor;
class Item;
class Faction;
class CanReserve;
class Simulation;
struct ItemType;
struct DeserializationMemo;

class HasShape
{
	Simulation& m_simulation;
protected:
	HasShape(Simulation& simulation, const Shape& shape, bool st, Facing f = 0u, uint32_t mr = 1u) : m_simulation(simulation), m_static(st), m_shape(&shape), m_location(nullptr), m_facing(f), m_canLead(*this), m_canFollow(*this), m_reservable(mr), m_isUnderground(false)  {}
	HasShape(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	bool m_static;
public:
	const Shape* m_shape;
	Block* m_location;
	Facing m_facing; 
	std::unordered_set<Block*> m_blocks;
	//TODO: Adjacent blocks cache.
	CanLead m_canLead;
	CanFollow m_canFollow;
	OnDestroy m_onDestroy;
	Reservable m_reservable;
	bool m_isUnderground;

	void setShape(const Shape& shape);
	void setStatic(bool isTrue);
	void reserveOccupied(CanReserve& canReserve);
	bool isAdjacentTo(const HasShape& other) const;
	bool isAdjacentTo(Block& block) const;
	bool predicateForAnyOccupiedBlock(std::function<bool(const Block&)> predicate) const;
	bool predicateForAnyAdjacentBlock(std::function<bool(const Block&)> predicate) const;
	std::unordered_set<Block*> getAdjacentBlocks();
	std::unordered_set<HasShape*> getAdjacentHasShapes();
	std::vector<Block*> getAdjacentAtLocationWithFacing(const Block& block, Facing facing);
	std::vector<Block*> getBlocksWhichWouldBeOccupiedAtLocationAndFacing(Block& location, Facing facing);
	bool allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing, const Faction& faction) const;
	bool allOccupiedBlocksAreReservable(const Faction& faction) const;
	bool isAdjacentToAt(const Block& location, Facing facing, const HasShape& hasShape) const;
	DistanceInBlocks distanceTo(const HasShape& other) const;
	Block* getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Block&)>& predicate);
	Block* getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Block&)>& predicate);
	Block* getBlockWhichIsAdjacentWithPredicate(std::function<bool(const Block&)>& predicate);
	Block* getBlockWhichIsOccupiedWithPredicate(std::function<bool(const Block&)>& predicate);
	Item* getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Item&)>& predicate);
	Item* getItemWhichIsAdjacentWithPredicate(std::function<bool(const Item&)>& predicate);
	Simulation& getSimulation() { return m_simulation; }
	EventSchedule& getEventSchedule();
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
