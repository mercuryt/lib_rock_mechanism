/*
 * A shared base class for Actor and Item.
 */
#pragma once

#include "deserializationMemo.h"
#include "faction.h"
#include "shape.h"
#include "leadAndFollow.h"
#include "onDestroy.h"
#include "reservable.h"

#include <unordered_set>

class Block;
struct MoveType;
class BlockHasShapes;
class Actor;
class Item;
struct Faction;
class CanReserve;
class Simulation;
class HasShape
{
	Simulation& m_simulation;
protected:
	HasShape(Simulation& simulation, const Shape& shape, bool st, uint8_t f = 0u, uint32_t mr = 1u) : m_simulation(simulation), m_static(st), m_shape(&shape), m_location(nullptr), m_facing(f), m_canLead(*this), m_canFollow(*this), m_reservable(mr), m_isUnderground(false)  {}
	HasShape(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	bool m_static;
public:
	const Shape* m_shape;
	Block* m_location;
	uint8_t m_facing; 
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
	std::vector<Block*> getAdjacentAtLocationWithFacing(const Block& block, uint8_t facing);
	std::vector<Block*> getBlocksWhichWouldBeOccupiedAtLocationAndFacing(Block& location, Facing facing);
	bool allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing, const Faction& faction) const;
	bool allOccupiedBlocksAreReservable(const Faction& faction) const;
	bool isAdjacentToAt(const Block& location, Facing facing, const HasShape& hasShape) const;
	uint32_t distanceTo(const HasShape& other) const;
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
	virtual uint32_t getMass() const = 0;
	virtual uint32_t getVolume() const = 0;
	virtual void setLocation(Block& block) = 0;
	virtual void exit() = 0;
	virtual const MoveType& getMoveType() const = 0;
	virtual uint32_t singleUnitMass() const = 0;
	friend class BlockHasShapes;
	// For debugging.
	virtual void log() const;
};
class BlockHasShapes
{
	Block& m_block;
	std::unordered_map<HasShape*, uint32_t> m_shapes;
	uint32_t m_totalVolume;
	uint32_t m_staticVolume;
	// Move costs cache is a structure made up of shape, moveType, and a vector of blocks which can be moved to from here, along with their move cost.
	// TODO: Store a pointer to the relevent section of move cost cache for adjacent blocks.
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<Block*, uint32_t>>>> m_moveCostsCache;
	void record(HasShape& hasShape, uint32_t volume);
	void remove(HasShape& hasShape);
	uint32_t getVolume(const HasShape& hasShape) const;
	uint32_t getVolumeIfStatic(const HasShape& hasShape) const;
public:
	BlockHasShapes(Block& b) : m_block(b), m_totalVolume(0), m_staticVolume(0)  { }
	void clearCache();
	void enter(HasShape& hasShape);
	void exit(HasShape& hasShape);
	void addQuantity(HasShape& hasShape, uint32_t quantity);
	void removeQuantity(HasShape& hasShape, uint32_t quantity);
	void tryToCacheMoveCosts(const Shape& shape, const MoveType& moveType, std::vector<std::pair<Block*, uint32_t>>& moveCosts);
	bool anythingCanEnterEver() const;
	bool canEnterEverFrom(const HasShape& shape, const Block& block) const;
	bool canEnterEverWithFacing(const HasShape& hasShape, const uint8_t facing) const;
	bool canEnterEverWithAnyFacing(const HasShape& hasShape) const;
	std::pair<bool, Facing> canEnterCurrentlyWithAnyFacingReturnFacing(const HasShape& hasShape) const;
	bool shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, const Block& block) const;
	bool shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const uint8_t facing) const;
	bool moveTypeCanEnter(const MoveType& moveType) const;
	bool moveTypeCanEnterFrom(const MoveType& moveType, const Block& from) const;
	bool canEnterCurrentlyFrom(const HasShape& hasShape, const Block& block) const;
	bool canEnterCurrentlyWithFacing(const HasShape& hasShape, const uint8_t& facing) const;
	bool canEnterCurrentlyWithAnyFacing(const HasShape& hasShape) const;
	// Durring pathing the algorithm will first check if cached move costs exist, if so it will use them.
	// If not it will make a new set of costs and store them within it's self.
	// During the next write phase the newly made costs will be added to the cache via ::tryToCacheMoveCosts.
	bool hasCachedMoveCosts(const Shape& shape, const MoveType& moveType) const;
	const std::vector<std::pair<Block*, uint32_t>>& getCachedMoveCosts(const Shape& shape, const MoveType& moveType) const;
	const std::vector<std::pair<Block*, uint32_t>> makeMoveCosts(const Shape& shape, const MoveType& moveType) const;
	uint32_t moveCostFrom(const MoveType& moveType, const Block& from) const;
	bool canStandIn() const;
	uint32_t getTotalVolume() const;
	bool contains(HasShape& hasShape) const { return m_shapes.contains(&hasShape); }
	const uint32_t& getStaticVolume() const { return m_staticVolume; }
	std::unordered_map<HasShape*, uint32_t>& getShapes() { return m_shapes; }
	friend class HasShape;
	// For testing.
	[[maybe_unused, nodiscard]] bool moveCostCacheIsEmpty() const { return m_moveCostsCache.empty(); }
};
inline void to_json(Json& data, const HasShape* const& hasShape){ data = reinterpret_cast<uintptr_t>(hasShape); }