#include "actors.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../space/space.h"
#include "../reservable.hpp"
void Actors::canReserve_clearAll(const ActorIndex& index)
{
	m_canReserve[index]->deleteAllWithoutCallback();
}
void Actors::canReserve_setFaction(const ActorIndex& index, FactionId faction)
{
	m_canReserve[index]->setFaction(faction);
}
void Actors::canReserve_reserveLocation(const ActorIndex& index, const Point3D& point, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	m_area.getSpace().reserve(point, canReserve_get(index), std::move(callback));
}
void Actors::canReserve_reserveItem(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	m_area.getItems().reservable_reserve(item, canReserve_get(index), quantity, std::move(callback));
}
bool Actors::canReserve_translateAndReservePositions(const ActorIndex& index, SmallMap<Point3D, std::unique_ptr<DishonorCallback>>&& pointsAndCallbacks, const Point3D& prevousPivot, const Point3D& newPivot, const Facing4& previousFacing, const Facing4& newFacing)
{
	return m_canReserve[index]->translateAndReservePositions(m_area.getSpace(), std::move(pointsAndCallbacks), prevousPivot, newPivot, previousFacing, newFacing);
}
bool Actors::canReserve_tryToReserveLocation(const ActorIndex& index, const Point3D& point, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	if(m_area.getSpace().isReserved(point, getFaction(index)))
		return false;
	canReserve_reserveLocation(index, point, std::move(callback));
	return true;
}
bool Actors::canReserve_tryToReserveItem(const ActorIndex& index, const ItemIndex& item, const Quantity& quantity, std::unique_ptr<DishonorCallback> callback)
{
	if(callback == nullptr)
		callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_area, m_area.getActors().m_referenceData.getReference(index));
	if(m_area.getItems().reservable_isFullyReserved(item, m_faction[index]))
		return false;
	canReserve_reserveItem(index, item, quantity, std::move(callback));
	return true;
}
SmallMap<Point3D, std::unique_ptr<DishonorCallback>> Actors::canReserve_unreserveAndReturnPointsAndCallbacksOnSameDeck(const ActorIndex& index)
{
	DeckId deckId = m_area.m_decks.queryDeckId(m_location[index]);
	if(deckId.empty())
		return {};
	auto condition = [&](const Point3D& point) { return m_area.m_decks.queryDeckId(point) == deckId; };
	return m_canReserve[index]->unreserveAndReturnPointsAndCallbacksWithCondition(condition);
}

bool Actors::canReserve_hasReservationWith(const ActorIndex& index, Reservable& reservable) const
{
	return m_canReserve[index]->hasReservationWith(reservable);
}
bool Actors::canReserve_canReserveLocation(const ActorIndex& index, const Point3D& location, const Facing4& facing) const
{
	const Space& space = m_area.getSpace();
	FactionId faction = m_faction[index];
	assert(faction.exists());
	const CuboidSet& cuboidSet = Shape::getCuboidsOccupiedAt(m_shape[index], space, location, facing);
	return !space.isReservedAny(cuboidSet, faction);
}
bool Actors::canReserve_locationAtEndOfPathIsUnreserved(const ActorIndex& index, const SmallSet<Point3D>& path) const
{
	assert(!path.empty());
	Facing4 facing;
	if(path.size() == 1)
		facing = m_location[index].getFacingTwords(path.back());
	else
		facing = (path.end() - 2)->getFacingTwords(path.back());
	return canReserve_canReserveLocation(index, path.back(), facing);
}
CanReserve& Actors::canReserve_get(const ActorIndex& index)
{
	return *m_canReserve[index];
}