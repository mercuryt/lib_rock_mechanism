#pragma once

#include "../types.h"
#include "../config.h"
#include <unordered_map>
#include <vector>
class BlockIndex;
class HasShape;
class Shape;
struct MoveType;
struct ItemType;
class BlockHasShapes
{
	std::unordered_map<HasShape*, CollisionVolume> m_shapes;
	// Move costs cache is a structure made up of shape, moveType, and a vector of blocks which can be moved to from here, along with their move cost.
	// TODO: replace with monolithic bitset
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<BlockIndex*, MoveCost>>>> m_moveCostsCache;
	BlockIndex& m_block;
	CollisionVolume m_dynamicVolume;
	CollisionVolume m_staticVolume;
	void record(HasShape& hasShape, CollisionVolume volume);
	void remove(HasShape& hasShape);
	CollisionVolume getVolume(const HasShape& hasShape) const;
	CollisionVolume getVolumeIfStatic(const HasShape& hasShape) const;
public:
	BlockHasShapes(BlockIndex& b) : m_block(b), m_dynamicVolume(0), m_staticVolume(0)  { }
	void clearCache();
	void enter(HasShape& hasShape);
	void exit(HasShape& hasShape);
	void addQuantity(HasShape& hasShape, uint32_t quantity);
	void removeQuantity(HasShape& hasShape, uint32_t quantity);
	void tryToCacheMoveCosts(const Shape& shape, const MoveType& moveType, std::vector<std::pair<BlockIndex*, MoveCost>>& moveCosts);
	[[nodiscard]] bool anythingCanEnterEver() const;
	// CanEnter methods which are not prefixed with static are to be used only for dynamic shapes.
	[[nodiscard]] bool canEnterEverFrom(const HasShape& shape, const BlockIndex& block) const;
	[[nodiscard]] bool canEnterEverWithFacing(const HasShape& hasShape, const Facing facing) const;
	[[nodiscard]] bool canEnterEverWithAnyFacing(const HasShape& hasShape) const;
	[[nodiscard]] bool canEnterCurrentlyFrom(const HasShape& hasShape, const BlockIndex& block) const;
	[[nodiscard]] bool canEnterCurrentlyWithFacing(const HasShape& hasShape, const Facing& facing) const;
	[[nodiscard]] bool canEnterCurrentlyWithAnyFacing(const HasShape& hasShape) const;
	[[nodiscard]] std::pair<bool, Facing> canEnterCurrentlyWithAnyFacingReturnFacing(const HasShape& hasShape) const;
	// Shape and move type can enter methods are used by leadAndFollow chains to combine the worst movetype with the largest shape for pathing.
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, const BlockIndex& block) const;
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const Facing facing) const;
	[[nodiscard]] bool shapeAndMoveTypeCanEnterEverWithAnyFacing(const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] bool shapeAndMoveTypeCanEnterCurrentlyWithFacing(const Shape& shape, const MoveType& moveType, const Facing facing) const;
	[[nodiscard]] bool moveTypeCanEnter(const MoveType& moveType) const;
	[[nodiscard]] bool moveTypeCanEnterFrom(const MoveType& moveType, const BlockIndex& from) const;
	[[nodiscard]] bool moveTypeCanBreath(const MoveType& moveType) const;
	// Static shapes are items or actors who are laying on the ground immobile.
	// They do not collide with dynamic shapes and have their own volume data.
	[[nodiscard]] bool staticCanEnterCurrentlyWithFacing(const HasShape& hasShape, const Facing& facing) const;
	[[nodiscard]] bool staticCanEnterCurrentlyWithAnyFacing(const HasShape& hasShape) const;
	[[nodiscard]] std::pair<bool, Facing> staticCanEnterCurrentlyWithAnyFacingReturnFacing(const HasShape& hasShape) const;
	[[nodiscard]] bool staticShapeCanEnterWithFacing(const Shape& shape, Facing facing) const;
	[[nodiscard]] bool staticShapeCanEnterWithAnyFacing(const Shape& shape) const;
	// Durring pathing the algorithm will first check if cached move costs exist, if so it will use them.
	// If not it will make a new set of costs and store them within it's self.
	// During the next write phase the newly made costs will be added to the cache via ::tryToCacheMoveCosts.
	[[nodiscard]] bool hasCachedMoveCosts(const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] const std::vector<std::pair<BlockIndex*, MoveCost>>& getCachedMoveCosts(const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] const std::vector<std::pair<BlockIndex*, MoveCost>> makeMoveCosts(const Shape& shape, const MoveType& moveType) const;
	// Get a move cost for moving from a block onto this one for a given move type.
	[[nodiscard]] MoveCost moveCostFrom(const MoveType& moveType, const BlockIndex& from) const;
	[[nodiscard]] bool canStandIn() const;
	[[nodiscard]] CollisionVolume getDynamicVolume() const;
	[[nodiscard]] bool contains(HasShape& hasShape) const { return m_shapes.contains(&hasShape); }
	[[nodiscard]] CollisionVolume getStaticVolume() const { return m_staticVolume; }
	[[nodiscard]] std::unordered_map<HasShape*, CollisionVolume>& getShapes() { return m_shapes; }
	[[nodiscard]] uint32_t getQuantityOfItemWhichCouldFit(const ItemType& itemType) const;
	friend class HasShape;
	// For testing.
	[[maybe_unused, nodiscard]] bool moveCostCacheIsEmpty() const { return m_moveCostsCache.empty(); }
};
inline void to_json(Json& data, const HasShape* const& hasShape){ data = reinterpret_cast<uintptr_t>(hasShape); }
