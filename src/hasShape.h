/*
 * A shared base class for Actor and Item.
 */
#pragma once

#include "shape.h"
#include "leadAndFollow.h"

#include <unordered_set>

class Block;
struct MoveType;
class BlockHasShapes;

class HasShape
{
protected:
	HasShape(const Shape& s, bool st, uint8_t f = 0) : m_static(st), m_shape(&s), m_location(nullptr), m_facing(f), m_canLead(*this), m_canFollow(*this) {}
	bool m_static;
public:
	const Shape* m_shape;
	Block* m_location;
	uint8_t m_facing; 
	std::unordered_set<Block*> m_blocks;
	CanLead m_canLead;
	CanFollow m_canFollow;
	bool m_isUnderground;
	bool isAdjacentTo(HasShape& other) const;
	bool isAdjacentTo(Block& block) const;
	bool predicateForAnyOccupiedBlock(std::function<bool(const Block&)> predicate);
	void setStatic(bool isTrue);
	std::unordered_set<Block*> getOccupiedAndAdjacent();
	std::unordered_set<Block*> getAdjacentBlocks();
	std::unordered_set<HasShape*> getAdjacentHasShapes();
	virtual bool isItem() const = 0;
	virtual bool isActor() const = 0;
	virtual uint32_t getMass() const = 0;
	virtual uint32_t getVolume() const = 0;
	virtual void setLocation(Block& block) = 0;
	virtual void exit() = 0;
	virtual const MoveType& getMoveType() const = 0;
	virtual uint32_t singleUnitMass() const = 0;
	friend class BlockHasShapes;
};
class BlockHasShapes
{
	Block& m_block;
	std::unordered_map<HasShape*, uint32_t> m_shapes;
	uint32_t m_totalVolume;
	uint32_t m_staticVolume;
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
	bool shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, const Block& block) const;
	bool shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const uint8_t facing) const;
	bool moveTypeCanEnter(const MoveType& moveType) const;
	bool moveTypeCanEnterFrom(const MoveType& moveType, const Block& from) const;
	bool canEnterCurrentlyFrom(const HasShape& hasShape, const Block& block) const;
	bool canEnterCurrentlyWithFacing(const HasShape& hasShape, const uint8_t& facing) const;
	const std::vector<std::pair<Block*, uint32_t>>& getMoveCosts(const Shape& shape, const MoveType& moveType);
	uint32_t moveCostFrom(const MoveType& moveType, const Block& from) const;
	bool canStandIn() const;
	uint32_t getTotalVolume() const;
	bool contains(HasShape& hasShape) const { return m_shapes.contains(&hasShape); }
	const uint32_t& getStaticVolume() const { return m_staticVolume; }
	std::unordered_map<HasShape*, uint32_t>& getShapes() { return m_shapes; }
	friend class HasShape;
};
