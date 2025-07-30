#include "deck.h"
#include "area/area.h"
#include "space/space.h"
#include "actors/actors.h"
#include "items/items.h"
#include "reservable.hpp"
void AreaHasDecks::updatePoints(Area& area, const DeckId& id)
{
	for(const Cuboid& cuboid : m_data[id].cuboidSet)
	{
		area.m_hasTerrainFacades.update(cuboid);
		m_pointData.maybeInsert(cuboid, id);
	}
}
void AreaHasDecks::clearPoints(Area& area, const DeckId& id)
{
	for(const Cuboid& cuboid : m_data[id].cuboidSet)
	{
		m_pointData.maybeRemove(cuboid);
		area.m_hasTerrainFacades.update(cuboid);
	}
}
[[nodiscard]] DeckId AreaHasDecks::registerDecks(Area& area, CuboidSet& decks, const ActorOrItemIndex& actorOrItemIndex)
{
	m_data.emplace(m_nextId, CuboidSetAndActorOrItemIndex{decks, actorOrItemIndex});
	updatePoints(area, m_nextId);
	auto output = m_nextId;
	++m_nextId;
	return output;
}
void AreaHasDecks::unregisterDecks(Area& area, const DeckId& id)
{
	clearPoints(area, id);
	m_data.erase(id);
}
void AreaHasDecks::shift(Area& area, const DeckId& id, const Offset3D& offset, const Distance& distance, const Point3D& origin, const Facing4& oldFacing, const Facing4& newFacing)
{
	clearPoints(area, id);
	if(oldFacing != newFacing)
	{
		int facingChange = (int)oldFacing - (int)newFacing;
		if(facingChange < 0)
			facingChange += 4;
		m_data[id].cuboidSet.rotateAroundPoint(origin, (Facing4)facingChange);
	}
	m_data[id].cuboidSet.shift(offset, distance);
	updatePoints(area, id);
}
void DeckRotationData::rollback(Area& area, SmallMap<ActorOrItemIndex, DeckRotationDataSingle>::iterator begin, SmallMap<ActorOrItemIndex, DeckRotationDataSingle>::iterator end)
{
	for(auto& iter = begin; iter != end; ++iter)
		iter->first.location_clear(area);
}
DeckRotationData DeckRotationData::recordAndClearDependentPositions(Area& area, const ActorOrItemIndex& actorOrItem)
{
	DeckRotationData output;
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	Space& space = area.getSpace();
	SmallSet<ActorOrItemIndex> actorsOrItemsOnDeck;
	SmallSet<Project*> projectsOnDeck;
	SmallSet<Point3D> pointsContainingFluid;
	if(actorOrItem.isActor())
	{
		const ActorIndex& actor = actorOrItem.getActor();
		actorsOrItemsOnDeck = actors.onDeck_get(actor);
	}
	else
	{
		const ItemIndex& item = actorOrItem.getItem();
		actorsOrItemsOnDeck = items.onDeck_get(item);
		projectsOnDeck = items.onDeck_getProjects(item);
		pointsContainingFluid = items.onDeck_getPointsContainingFluid(item);
	}
	for(const ActorOrItemIndex& onDeck : actorsOrItemsOnDeck)
	{
		if(onDeck.isActor())
		{
			ActorIndex actor = onDeck.getActor();
			// TODO: this fetches the DeckId seperately for each actor but they are all the same.
			DeckRotationDataSingle data{actors.canReserve_unreserveAndReturnPointsAndCallbacksOnSameDeck(actor), actors.getLocation(actor), actors.getFacing(actor)};
			output.m_actorsOrItems.insert(onDeck, std::move(data));
			actors.location_clear(actor);
		}
		else
		{
			const ItemIndex& item = onDeck.getItem();
			output.m_actorsOrItems.insert(onDeck, DeckRotationDataSingle{{}, items.getLocation(item), items.getFacing(item)});
			items.location_clear(item);
		}
	}
	if(actorOrItem.isItem())
	{
		const ItemIndex& item = actorOrItem.getItem();
		if(items.onDeck_hasDecks(item))
		{
			const DeckId& deckId = area.m_decks.getForPoint(items.getLocation(item));
			if(deckId.exists())
			{
				for(Project* project : projectsOnDeck)
				{
					auto condition = [&](const Point3D& point) { return area.m_decks.getForPoint(point) == deckId; };
					output.m_projects.insert(project, {project->getCanReserve().unreserveAndReturnPointsAndCallbacksWithCondition(condition), project->getLocation(), Facing4::Null});
					project->clearLocation();
				}
				for(const Point3D& point : pointsContainingFluid)
				{
					output.m_fluids.insert(point, {});
					auto& storedData = output.m_fluids.back().second;
					for(const FluidData& fluidData : space.fluid_getAll(point))
						storedData.insert(const_cast<FluidTypeId&>(fluidData.type), const_cast<CollisionVolume&>(fluidData.volume));
					space.fluid_removeAllSyncronus(point);
				}
			}
		}
	}
	return output;
}
void DeckRotationData::reinstanceAtRotatedPosition(Area& area, const Point3D& previousPivot, const Point3D& newPivot, const Facing4& previousFacing, const Facing4& newFacing)
{
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	SmallSet<ActorIndex> actorsWhichCannotReserveRotatedPosition;
	SmallSet<Project*> projectsWhichCannotReserveRotatedPosition;
	// Reinstance actors and items.
	for(auto& [onDeck, data] : m_actorsOrItems)
	{
		if(onDeck.isActor())
		{
			const ActorIndex& actor = onDeck.getActor();
			// Update location.
			const Point3D location = data.location.translate(previousPivot, newPivot, previousFacing, newFacing);
			const Facing4 facing = util::rotateFacingByDifference(data.facing, previousFacing, newFacing);
			actors.location_set(actor, location, facing);
			if(actors.mount_isPilot(actor))
				continue;
			// Update path.
			auto& path = actors.move_getPath(actor);
			if(!path.empty())
			{
				uint i = 0;
				for(const Point3D& point : path)
				{
					path[i] = point.translate(previousPivot, newPivot, previousFacing, newFacing);
					++i;
				}
			}
			// Update destination.
			const Point3D& destination = actors.move_getDestination(actor);
			if(destination.exists())
			{
				const Point3D newDestination = destination.translate(previousPivot, newPivot, previousFacing, newFacing);
				actors.move_updateDestination(actor, newDestination);
			}
			// Update reservations.
			bool reservationsSuccessful = actors.canReserve_translateAndReservePositions(actor, std::move(data.reservedPointsAndCallbacks), previousPivot, newPivot, previousFacing, newFacing);
			if(!reservationsSuccessful)
				actorsWhichCannotReserveRotatedPosition.insert(actor);
		}
		else
		{
			// Item.
			const ItemIndex& item = onDeck.getItem();
			const Point3D location = data.location.translate(previousPivot, newPivot, previousFacing, newFacing);
			const Facing4 facing = util::rotateFacingByDifference(data.facing, previousFacing, newFacing);
			items.location_set(item, location, facing);
		}
	}
	// Reinstance projects.
	for(auto& [project, data] : m_projects)
	{
		bool reservationsSuccessful = project->getCanReserve().translateAndReservePositions(space, std::move(data.reservedPointsAndCallbacks), previousPivot, newPivot, previousFacing, newFacing);
		if(!reservationsSuccessful)
			projectsWhichCannotReserveRotatedPosition.insert(project);
		project->setLocation(data.location);
	}
	// Reinstance fluids.
	for(const auto& [point, fluidData] : m_fluids)
	{
		const Point3D location = point.translate(previousPivot, newPivot, previousFacing, newFacing);
		for(const auto& [fluidType, volume] : fluidData)
			space.fluid_add(location, volume, fluidType);
	}
	// If an actor cannot reserve the rotated positions they must reset their objective.
	for(const ActorIndex& actor : actorsWhichCannotReserveRotatedPosition)
		actors.objective_canNotCompleteSubobjective(actor);
	// If a project cannot reserve the rotated positions it must reset if it can or otherwise cancel.
	// Resetable projects are typically those created directly by the player, so they should not be cancled.
	for(Project* project : projectsWhichCannotReserveRotatedPosition)
		project->resetOrCancel();
}
SetLocationAndFacingResult DeckRotationData::tryToReinstanceAtRotatedPosition(Area& area, const Point3D& previousPivot, const Point3D& newPivot, const Facing4& previousFacing, const Facing4& newFacing)
{
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	SmallSet<ActorIndex> actorsWhichCannotReserveRotatedPosition;
	SmallSet<Project*> projectsWhichCannotReserveRotatedPosition;
	// Reinstance actors and item
	auto& actorsOrItems = m_actorsOrItems;
	const auto& end = actorsOrItems.end();
	for(decltype(m_actorsOrItems)::iterator iter = actorsOrItems.begin(); iter != end; ++iter)
	{
		const ActorOrItemIndex& onDeck = iter->first;
		const DeckRotationDataSingle& data = iter->second;
		if(onDeck.isActor())
		{
			const ActorIndex& actor = onDeck.getActor();
			// Update location.
			// If setting location fails then we rollback all locations set thus far and return the failing status.
			const Point3D location = data.location.translate(previousPivot, newPivot, previousFacing, newFacing);
			const Facing4 facing = util::rotateFacingByDifference(data.facing, previousFacing, newFacing);
			SetLocationAndFacingResult result = actors.location_tryToSet(actor, location, facing);
			if(result != SetLocationAndFacingResult::Success)
			{
				rollback(area, actorsOrItems.begin(), iter);
				return result;
			}
			// Pilot's paths and reservations are not relative to the deck.
			if(actors.mount_isPilot(actor))
				continue;
			// Update path.
			auto& path = actors.move_getPath(actor);
			if(!path.empty())
			{
				uint i = 0;
				for(const Point3D& point : path)
				{
					path[i] = point.translate(previousPivot, newPivot, previousFacing, newFacing);
					++i;
				}
			}
			// Update destination.
			const Point3D& destination = actors.move_getDestination(actor);
			if(destination.exists())
			{
				const Point3D newDestination = destination.translate(previousPivot, newPivot, previousFacing, newFacing);
				actors.move_updateDestination(actor, newDestination);
			}
			// Update reservations.
			bool reservationsSuccessful = actors.canReserve_translateAndReservePositions(actor, std::move(iter->second.reservedPointsAndCallbacks), previousPivot, newPivot, previousFacing, newFacing);
			if(!reservationsSuccessful)
				actorsWhichCannotReserveRotatedPosition.insert(actor);
		}
		else
		{
			// Item.
			const ItemIndex& item = onDeck.getItem();
			const Point3D location = data.location.translate(previousPivot, newPivot, previousFacing, newFacing);
			const Facing4 facing = util::rotateFacingByDifference(data.facing, previousFacing, newFacing);
			// If setting location fails then we rollback all locations set thus far and return the failing status.
			std::pair<ItemIndex, SetLocationAndFacingResult> result = items.location_tryToSet(item, location, facing);
			// TODO: check if combine?
			if(result.second != SetLocationAndFacingResult::Success)
			{
				rollback(area, actorsOrItems.begin(), iter);
				return result.second;
			}
		}
	}
	// Reinstance projects.
	for(auto& [project, data] : m_projects)
	{
		bool reservationsSuccessful = project->getCanReserve().translateAndReservePositions(space, std::move(data.reservedPointsAndCallbacks), previousPivot, newPivot, previousFacing, newFacing);
		if(!reservationsSuccessful)
			projectsWhichCannotReserveRotatedPosition.insert(project);
		project->setLocation(data.location);
	}
	// Reinstance fluids.
	for(const auto& [point, fluidData] : m_fluids)
	{
		const Point3D location = point.translate(previousPivot, newPivot, previousFacing, newFacing);
		for(const auto& [fluidType, volume] : fluidData)
			space.fluid_add(location, volume, fluidType);
	}
	// If an actor cannot reserve the rotated positions they must reset their objective.
	for(const ActorIndex& actor : actorsWhichCannotReserveRotatedPosition)
		actors.objective_canNotCompleteSubobjective(actor);
	// If a project cannot reserve the rotated positions it must reset if it can or otherwise cancel.
	// Resetable projects are typically those created directly by the player, so they should not be cancled.
	for(Project* project : projectsWhichCannotReserveRotatedPosition)
		project->resetOrCancel();
	return SetLocationAndFacingResult::Success;
}