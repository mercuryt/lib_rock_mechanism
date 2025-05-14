#pragma once
#include "geometry/cuboidSet.h"
#include "strongInteger.h"
#include "types.h"
#include "actorOrItemIndex.h"
#include "../lib/json.hpp"

class Project;

struct CuboidSetAndActorOrItemIndex
{
	CuboidSet cuboidSet;
	ActorOrItemIndex actorOrItemIndex;
};
class AreaHasDecks
{
	SmallMap<DeckId, CuboidSetAndActorOrItemIndex> m_data;
	StrongVector<DeckId, BlockIndex> m_blockData;
	DeckId m_nextId = DeckId::create(0);
public:
	AreaHasDecks(Area& area);
	void updateBlocks(Area& area, const DeckId& id);
	void clearBlocks(Area& area, const DeckId& id);
	[[nodiscard]] DeckId registerDecks(Area& area, CuboidSet& decks, const ActorOrItemIndex& actorOrItemIndex);
	void unregisterDecks(Area& area, const DeckId& id);
	void shift(Area& area, const DeckId& id, const Offset3D& offset, const DistanceInBlocks& distance, const BlockIndex& origin, const Facing4& oldFacing, const Facing4& newFacing);
	[[nodiscard]] bool blockIsPartOfDeck(const BlockIndex& block) const { return m_blockData[block].exists(); }
	[[nodiscard]] DeckId getForBlock(const BlockIndex& block) const { return m_blockData[block]; }
	[[nodiscard]] ActorOrItemIndex getForId(const DeckId& id) { return m_data[id].actorOrItemIndex; }
};
// To be used to store data when moving an item with traversable decks.
// We must remove all reservations, shapes, fluids, and projects prior to moving the item, and recreate them in their new locations after.
struct DeckRotationDataSingle
{
	SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> reservedBlocksAndCallbacks;
	BlockIndex location;
	Facing4 facing;
	DeckRotationDataSingle() = default;
	DeckRotationDataSingle(SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>>&& rbac, BlockIndex l, Facing4 f) :
		reservedBlocksAndCallbacks(std::move(rbac)),
		location(l),
		facing(f)
	{ }
	DeckRotationDataSingle(const DeckRotationDataSingle&) = delete;
	DeckRotationDataSingle(DeckRotationDataSingle&& other) noexcept :
		reservedBlocksAndCallbacks(std::move(other.reservedBlocksAndCallbacks)),
		location(other.location),
		facing(other.facing)
	{ }
	DeckRotationDataSingle& operator=(const DeckRotationDataSingle&) = delete;
	DeckRotationDataSingle& operator=(DeckRotationDataSingle&& other) noexcept
	{
		reservedBlocksAndCallbacks = std::move(other.reservedBlocksAndCallbacks);
		location = other.location;
		facing = other.facing;
		return *this;
	}
};
class DeckRotationData
{
	SmallMap<Project*, DeckRotationDataSingle> m_projects;
	SmallMap<ActorOrItemIndex, DeckRotationDataSingle> m_actorsOrItems;
	SmallMap<BlockIndex, SmallMap<FluidTypeId, CollisionVolume>> m_fluids;
	void rollback(Area& area, SmallMap<ActorOrItemIndex, DeckRotationDataSingle>::iterator begin, SmallMap<ActorOrItemIndex, DeckRotationDataSingle>::iterator end);
public:
	static DeckRotationData recordAndClearDependentPositions(Area& area, const ActorOrItemIndex& actorOrItem);
	void reinstanceAtRotatedPosition(Area& area, const BlockIndex& previousPivot, const BlockIndex& newPivot, const Facing4& previousFacing, const Facing4& newFacing);
	SetLocationAndFacingResult tryToReinstanceAtRotatedPosition(Area& area, const BlockIndex& previousPivot, const BlockIndex& newPivot, const Facing4& previousFacing, const Facing4& newFacing);
};