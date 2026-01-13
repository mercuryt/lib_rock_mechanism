#pragma once
#include "geometry/cuboidSet.h"
#include "geometry/mapWithCuboidKeys.h"
#include "numericTypes/types.h"
#include "actorOrItemIndex.h"
#include "dataStructures/smallMap.h"
#include "dataStructures/rtreeData.h"
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
	RTreeData<DeckId> m_pointData;
	DeckId m_nextId = DeckId::create(0);
public:
	void updatePoints(Area& area, const DeckId& id);
	void clearPoints(Area& area, const DeckId& id);
	[[nodiscard]] DeckId registerDecks(Area& area, const CuboidSet& decks, const ActorOrItemIndex& actorOrItemIndex);
	void unregisterDecks(Area& area, const DeckId& id);
	void shift(Area& area, const DeckId& id, const Offset3D& offset, const Distance& distance, const Point3D& origin, const Facing4& oldFacing, const Facing4& newFacing);
	[[nodiscard]] bool isPartOfDeck(const auto& shape) const { return m_pointData.queryAny(shape); }
	[[nodiscard]] DeckId queryDeckId(const auto& shape) const { return m_pointData.queryGetOne(shape); }
	[[nodiscard]] ActorOrItemIndex getForId(const DeckId& id) { return m_data[id].actorOrItemIndex; }
};
// To be used to store data when moving an item with traversable decks.
// We must remove all reservations, shapes, fluids, and projects prior to moving the item, and recreate them in their new locations after.
struct DeckRotationDataSingle
{
	SmallMap<Point3D, std::unique_ptr<DishonorCallback>> reservedPointsAndCallbacks;
	Point3D location;
	Facing4 facing;
	DeckRotationDataSingle() = default;
	DeckRotationDataSingle(SmallMap<Point3D, std::unique_ptr<DishonorCallback>>&& rbac, Point3D l, Facing4 f) :
		reservedPointsAndCallbacks(std::move(rbac)),
		location(l),
		facing(f)
	{ }
	DeckRotationDataSingle(const DeckRotationDataSingle&) = delete;
	DeckRotationDataSingle(DeckRotationDataSingle&& other) noexcept :
		reservedPointsAndCallbacks(std::move(other.reservedPointsAndCallbacks)),
		location(other.location),
		facing(other.facing)
	{ }
	DeckRotationDataSingle& operator=(const DeckRotationDataSingle&) = delete;
	DeckRotationDataSingle& operator=(DeckRotationDataSingle&& other) noexcept
	{
		reservedPointsAndCallbacks = std::move(other.reservedPointsAndCallbacks);
		location = other.location;
		facing = other.facing;
		return *this;
	}
};
class DeckRotationData
{
	SmallMap<Project*, DeckRotationDataSingle> m_projects;
	SmallMap<ActorOrItemIndex, DeckRotationDataSingle> m_actorsOrItems;
	MapWithCuboidKeys<std::pair<FluidTypeId, CollisionVolume>> m_fluids;
	void rollback(Area& area, SmallMap<ActorOrItemIndex, DeckRotationDataSingle>::iterator begin, SmallMap<ActorOrItemIndex, DeckRotationDataSingle>::iterator end);
public:
	static DeckRotationData recordAndClearDependentPositions(Area& area, const ActorOrItemIndex& actorOrItem);
	void reinstanceAtRotatedPosition(Area& area, const Point3D& previousPivot, const Point3D& newPivot, const Facing4& previousFacing, const Facing4& newFacing);
	SetLocationAndFacingResult tryToReinstanceAtRotatedPosition(Area& area, const Point3D& previousPivot, const Point3D& newPivot, const Facing4& previousFacing, const Facing4& newFacing);
};