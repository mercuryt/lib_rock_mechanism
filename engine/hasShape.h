/*
 * A shared base class for Actor and Item.
 */
#pragma once

#include "leadAndFollow.h"
#include "onDestroy.h"
#include "reservable.h"
#include "types.h"

#include <unordered_set>

class BlockHasShapes;
class Actor;
class Area;
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
public:
	Reservable m_reservable; // 6.5
	CanLead m_canLead; // 4
	CanFollow m_canFollow; // 4
	OnDestroy m_onDestroy; // 2
	std::unordered_set<BlockIndex> m_blocks;
private:
	Simulation& m_simulation;
protected:
	Faction* m_faction = nullptr;
public:
	const Shape* m_shape = nullptr;
	BlockIndex m_location = BLOCK_INDEX_MAX;
	Area* m_area = nullptr;
	Facing m_facing= 0; 
	//TODO: Adjacent blocks offset cache?
	bool m_isUnderground = false;
protected:
	bool m_static = false;
	HasShape(Simulation& simulation, const Shape& shape, bool isStatic, Facing f = 0u, uint32_t maxReservations = 1u, Faction* faction = nullptr) :
	       	m_reservable(maxReservations), m_canLead(*this), m_canFollow(*this, simulation), m_simulation(simulation), m_faction(faction), 
		m_shape(&shape), m_location(BLOCK_INDEX_MAX), m_facing(f), m_isUnderground(false), m_static(isStatic) { }
	HasShape(const Json& data, DeserializationMemo& deserializationMemo);
public:
	void setShape(const Shape& shape);
	void setStatic(bool isTrue);
	void reserveOccupied(CanReserve& canReserve);
	[[nodiscard]] Json toJson() const;
	// May return nullptr.
	[[nodiscard]] Faction* getFaction() const { return m_faction; }
	[[nodiscard]] bool isAdjacentTo(const HasShape& other) const;
	[[nodiscard]] bool isStatic() const { return m_static; }
	[[nodiscard]] bool isAdjacentTo(BlockIndex block) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlock(std::function<bool(BlockIndex)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentBlock(std::function<bool(BlockIndex)> predicate) const;
	[[nodiscard]] std::unordered_set<BlockIndex> getAdjacentBlocks();
	[[nodiscard]] const std::unordered_set<BlockIndex> getAdjacentBlocks() const;
	[[nodiscard]] std::unordered_set<HasShape*> getAdjacentHasShapes();
	[[nodiscard]] std::vector<BlockIndex> getAdjacentAtLocationWithFacing(BlockIndex block, Facing facing);
	[[nodiscard]] std::vector<BlockIndex> getBlocksWhichWouldBeOccupiedAtLocationAndFacing(BlockIndex location, Facing facing);
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(BlockIndex location, Facing facing, Faction& faction) const;
	[[nodiscard]] bool allOccupiedBlocksAreReservable(Faction& faction) const;
	[[nodiscard]] bool isAdjacentToAt(BlockIndex location, Facing facing, const HasShape& hasShape) const;
	[[nodiscard]] DistanceInBlocks distanceTo(const HasShape& other) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(BlockIndex location, Facing facing, std::function<bool(BlockIndex)>& predicate);
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(BlockIndex location, Facing facing, std::function<bool(BlockIndex)>& predicate);
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentWithPredicate(std::function<bool(BlockIndex)>& predicate);
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedWithPredicate(std::function<bool(BlockIndex)>& predicate);
	[[nodiscard]] Item* getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(BlockIndex location, Facing facing, std::function<bool(const Item&)>& predicate);
	[[nodiscard]] Item* getItemWhichIsAdjacentWithPredicate(std::function<bool(const Item&)>& predicate);
	[[nodiscard]] Simulation& getSimulation() { return m_simulation; }
	[[nodiscard]] EventSchedule& getEventSchedule();
	virtual bool isItem() const = 0;
	virtual bool isActor() const = 0;
	virtual bool isGeneric() const = 0;
	virtual Mass getMass() const = 0;
	virtual Volume getVolume() const = 0;
	virtual void setLocation(BlockIndex block, Area* area = nullptr) = 0;
	virtual void exit() = 0;
	virtual const MoveType& getMoveType() const = 0;
	virtual Mass singleUnitMass() const = 0;
	virtual Quantity getQuantity() const { return 1; }
	friend class BlockHasShapes;
	// For debugging.
	virtual void log() const;
};
