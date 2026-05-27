#include "stockpile.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../space/space.h"
#include "../path/areaHasPaths.hpp"
#include "../numericTypes/types.h"
// Objective Type.
bool StockPileObjectiveType::canBeAssigned(Area& area, const ActorIndex actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot stockpile.
	// TODO: allow pilots?
	if(actors.mount_exists(actor))
		return false;
	return area.m_hasStockPiles.getForFaction(actors.getFaction(actor)).isAnyHaulingAvailableFor(actor);
}
std::unique_ptr<Objective> StockPileObjectiveType::makeFor(Area&, const ActorIndex) const
{
	return std::make_unique<StockPileObjective>();
}
// Objective.
StockPileObjective::StockPileObjective() : Objective(Config::stockPilePriority) { }
StockPileObjective::StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo, Area&) :
	Objective(data, deserializationMemo),
	m_stockPileLocation(data["stockPileLocation"]),
	m_itemStartLocation(data["itemStartLocation"]),
	m_itemType(data["itemType"]),
	m_project(data.contains("project") ? static_cast<StockPileProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{
	if(data.contains("stockPileLocation"))
		m_stockPileLocation = data["stockPileLocation"].get<Point3D>();
	if(data.contains("hasCheckedForDropOffLocation"))
		m_hasCheckedForCloserDropOffLocation = true;
}
Json StockPileObjective::toJson() const
{
	Json data = Objective::toJson();
	data.update({
		{"stockPileLocation", m_stockPileLocation},
		{"itemStartLocation", m_itemStartLocation},
		{"itemType", m_itemType},
	});
	if(m_project != nullptr)
		data["project"] = *m_project;
	if(m_stockPileLocation.exists())
		data["stockPileLocation"] = m_stockPileLocation;
	if(m_hasCheckedForCloserDropOffLocation)
		data["hasCheckedForDropOffLocation"] = true;
	return data;
}
void StockPileObjective::execute(Area& area, const ActorIndex actor)
{
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
	{
		Actors& actors = area.getActors();
		assert(!actors.project_exists(actor));
		if(m_itemStartLocation.empty())
			actors.move_pathRequestRecord(actor, std::make_unique<StockPilePathRequest>(area, *this, actor));
		else
			actors.move_pathRequestRecord(actor, std::make_unique<StockPileDestinationPathRequest>(area, *this, actor));
	}
	else
		m_project->commandWorker(actor);
}
void StockPileObjective::cancel(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_project != nullptr)
	{
		m_project->removeWorker(actor);
		m_project = nullptr;
	}
	else
		assert(!actors.project_exists(actor));
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
void StockPileObjective::reset(Area& area, const ActorIndex actor)
{
	m_hasCheckedForCloserDropOffLocation = false;
	unsetItemAndDestination();
	cancel(area, actor);
}
void StockPileObjective::setItemAndDestination(Area& area, ItemIndex item, Point3D destination)
{
	Items& items = area.getItems();
	m_stockPileLocation = destination;
	m_itemStartLocation = items.getLocation(item);
	m_itemType = items.getItemType(item);
}
void StockPileObjective::unsetItemAndDestination()
{
	m_stockPileLocation.clear();
	m_itemStartLocation.clear();
	m_itemType.clear();
}
bool StockPileObjective::destinationCondition(Area& area, const Point3D point, const ItemIndex item, const ActorIndex actor)
{
	Space& space = area.getSpace();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	if(!space.item_empty(point))
		// Don't put multiple items in the same point unless they are generic and share item type and material type.
		if(!items.isGeneric(item) || space.item_getAll(point).size() != 1 || space.item_getCount(point, items.getItemType(item), items.getMaterialType(item)) == 0)
			return false;
	FactionId faction = actors.getFaction(actor);
	const StockPile* stockpile = space.stockpile_getOneForFaction(point, faction);
	if(stockpile == nullptr || !stockpile->isEnabled() || !space.stockpile_isAvalible(point, faction))
		return false;
	if(space.isReserved(point, faction))
		return false;
	if(!stockpile->accepts(item))
		return false;
	// TODO: this should be move out of this method so it can be marked const.
	m_stockPileLocation = point;
	return true;
};
ItemIndex StockPileObjective::getItem(const Area& area, ActorIndex actor) const
{
	assert(m_itemType.exists());
	const Space& space = area.getSpace();
	FactionId faction = area.getActors().getFaction(actor);
	const StockPile* stockPile = space.stockpile_getOneForFaction(m_stockPileLocation, faction);
	const Items& items = area.getItems();
	return area.getSpace().item_getOneWithCondition(m_itemStartLocation, [this, &items, stockPile](const ItemIndex item){
		return items.getItemType(item) == m_itemType && stockPile->accepts(item); });
}
// Path Reqests
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
StockPilePathRequest::StockPilePathRequest(Area& area, StockPileObjective& spo, const ActorIndex actorIndex) :
	m_objective(spo)
{
	assert(m_objective.m_project == nullptr);
	assert(!m_objective.hasDestination());
	assert(!m_objective.m_hasCheckedForCloserDropOffLocation);
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForStockPileItems;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
	anyOccupiedPoint = false;
	reserveDestination = false;
	// The path found might be either to the item or to the destination, so don't use it.
	// TODO: always return the path to the item even if the destination is found after it.
	returnPath = false;
}
PathResult StockPilePathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Actors& actors = area.getActors();
	const Space& space = area.getSpace();
	const Items& items = area.getItems();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	assert(!actors.project_exists(actorIndex));
	const FactionId& actorFaction = actors.getFaction(actorIndex);
	const auto& hasStockPiles = area.m_hasStockPiles.getForFaction(actorFaction);
	AreaHasSpaceDesignationsForFaction& designationsForFaction = area.m_spaceDesignations.getForFaction(actorFaction);
	// Has side effect: writes to m_pointsByStockPile
	auto shortRangeCondition = [this, actorIndex, actorFaction, &hasStockPiles, &space, &items, &area, &designationsForFaction](const Cuboid cuboid) -> Point3D
	{
		if(designationsForFaction.check(cuboid, SpaceDesignation::StockPileHaulFrom)) [[unlikely]]
		{
			for(ItemIndex item : space.item_getAll(cuboid))
			{
				// Items may be encountered more then once.
				if(m_items.contains(item))
					continue;
				if(items.reservable_isFullyReserved(item, actorFaction))
					// Cannot stockpile reserved items.
					continue;
				if(items.stockpile_canBeStockPiled(item, actorFaction))
				{
					// Item can be stockpiled, check if any of the stockpiles we have seen so far will accept it.
					for (const auto& [stockpile, points] : m_pointsByStockPile)
					{
						if(stockpile->accepts(item))
							for(const Cuboid stockpileCuboid : points)
								for(const Point3D stockpilePoint : stockpileCuboid)
									if(checkDestination(area, item, stockpilePoint))
									{
										// Success
										m_objective.setItemAndDestination(area, item, stockpilePoint);
										return stockpilePoint;
									}
					}
					// Item does not match any stockpile seen so far, store it to compare against future stockpiles.
					m_items.insert(item);
				}
			}
		}
		if(designationsForFaction.check(cuboid, SpaceDesignation::StockPileHaulTo)) [[unlikely]]
		{
			for(RTreeDataWrapper<StockPile*, nullptr> wrappedStockpile : space.stockpile_getAllForFaction(cuboid, actorFaction))
			{
				StockPile* stockPile = wrappedStockpile.get();
				CuboidSet intersection = stockPile->getCuboids().intersection(cuboid);
				auto foundStockPileWithPoints = m_pointsByStockPile.find(stockPile);
				if(foundStockPileWithPoints != m_pointsByStockPile.end() && foundStockPileWithPoints->second.contains(intersection))
					continue;
				// Don't stockpile in reserved locations.
				space.reservation_removeFromForFaction(intersection, actorFaction);
				if(intersection.empty())
					continue;
				// Don't stockpile in the same tile as non-generics.
				// For generics we will check that the type matches and that they can fit later, when looking at specific items.
				space.item_queryForEachWithCuboid(intersection.boundry(), [&items, &intersection](Cuboid itemCuboid, ItemIndex item){
					ItemTypeId itemType = items.getItemType(item);
					if(!ItemType::getIsGeneric(itemType))
						intersection.maybeRemove(itemCuboid);
				});
				if(intersection.empty())
					continue;
				// Don't stockpile in a location where max volume has been reached.
				space.shape_queryStaticVolumeForEachWithCuboids(intersection.boundry(), [&intersection](Cuboid volumeCuboid, CollisionVolume volume){
					if(volume >= Config::maxPointVolume)
						intersection.maybeRemove(volumeCuboid);
				});
				if(intersection.empty())
					continue;
				// Check if any items found so far can be delivered here.
				for(ItemIndex item : m_items)
				{
					if(stockPile->accepts(item))
						// Stockpile can take item, look for a spot to put it.
						for(const Cuboid intersectingCuboid : intersection)
							for(const Point3D point : intersectingCuboid)
								if(checkDestination(area, item, point))
								{
									// Success.
									m_objective.setItemAndDestination(area, item, point);
									return point;
								}
				}
				// Save potential destination points with a pointer to the stockpile.
				auto found = m_pointsByStockPile.find(stockPile);
				if(found == m_pointsByStockPile.end())
					m_pointsByStockPile.insert(stockPile, intersection);
				else
					found->second.maybeAddAll(intersection);
			}
		}
		return Point3D::null();
	};
	auto longRangeCondition = [&designationsForFaction](const Cuboid cuboid) -> bool {
		return
			designationsForFaction.check(cuboid, SpaceDesignation::StockPileHaulTo) ||
			designationsForFaction.check(cuboid, SpaceDesignation::StockPileHaulFrom);
	};
	constexpr bool anyOccupied = false;
	constexpr bool useAdjacent = true;
	Point3D result = hasPaths.accessableCondition<useAdjacent, anyOccupied>(longRangeCondition, shortRangeCondition, toParamaters(area));
	return {{}, result};
}
void StockPilePathRequest::writeStep(Area& area, bool)
{
	Actors& actors = area.getActors();
	const ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	const FactionId& actorFaction = actors.getFaction(actorIndex);
	if(!m_objective.m_itemStartLocation.exists())
		// No combination of item and destination found.
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
	else
	{
		assert(m_objective.m_stockPileLocation.exists());
		Items& items = area.getItems();
		const ItemIndex item = m_objective.getItem(area, actorIndex);
		if(
			item.empty() ||
			items.reservable_isFullyReserved(item, actorFaction) ||
			!m_objective.destinationCondition(area, m_objective.m_stockPileLocation, item, actorIndex)
		)
		{
			// Found destination is no longer valid or item has been reserved
			m_objective.unsetItemAndDestination();
			m_objective.m_stockPileLocation.clear();
			assert(m_objective.m_project == nullptr);
		}
		m_objective.execute(area, actorIndex);
	}
}
bool StockPilePathRequest::checkDestination(const Area& area, const ItemIndex item, const Point3D point) const
{
	const Space& space = area.getSpace();
	const Items& items = area.getItems();
	// Only stockpile in non empty point if it means stacking generics.
	if(!space.item_empty(point))
	{
		const ItemTypeId itemType = items.getItemType(item);
		if(!ItemType::getIsGeneric(itemType))
			return false;
		// item cannot stack with the item in this point, don't stockpile it here.
		if(space.item_getCount(point, itemType, items.getMaterialType(item)) == 0)
			return false;
	}
	const ShapeId singleQuantityShape = ItemType::getShape(items.getItemType(item));
	return
		space.shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(point, singleQuantityShape, items.getMoveType(item), items.getOccupied(item)) !=
		Facing4::Null;
}
StockPilePathRequest::StockPilePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json StockPilePathRequest::toJson() const
{
	Json output = static_cast<const PathRequest&>(*this);
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "stockpile";
	return output;
}
StockPileDestinationPathRequest::StockPileDestinationPathRequest(Area& area, StockPileObjective& spo, const ActorIndex actorIndex) :
	m_objective(spo)
{
	assert(m_objective.m_project == nullptr);
	// A destination exists but we want to search for a better one.
	assert(m_objective.hasDestination());
	assert(m_objective.hasItem());
	assert(!m_objective.m_hasCheckedForCloserDropOffLocation);
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForStockPiles;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
	reserveDestination = false;
}
PathResult StockPileDestinationPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	const Actors& actors = area.getActors();
	const ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	const ItemIndex item = m_objective.getItem(area, actorIndex);
	if(item.empty())
		// Target item has moved somehow, restart objective.
		// returning without a path will cause writeStep to call canNotCompleteSubobjective.
		return {{}, Point3D::null()};
	assert(!actors.project_exists(actorIndex));
	//TODO: m_objective.destinationCondition has side effects.
	const auto shortRangeCondition = [&area, &o = m_objective, item, actorIndex](const Cuboid cuboid) -> Point3D
	{
		for(const Point3D point : cuboid)
			// TODO: use cuboids instead of points to query here.
			if(o.destinationCondition(area, point, item, actorIndex))
				return point;
		return Point3D::null();
	};
	const Space& space = area.getSpace();
	auto longRangeCondition = [&space, f = faction, item](const Cuboid cuboid) -> bool
	{
		for(const RTreeDataWrapper<StockPile*, nullptr>& wrappedPointer : space.stockpile_getAllForFaction(cuboid, f))
		{
			StockPile& stockPile = *wrappedPointer.get();
			if(stockPile.accepts(item))
				return true;
		}
		return false;
	};
	constexpr bool anyOccupied = false;
	constexpr bool anyAdjacent = true;
	return hasPaths.pathToCondition<anyAdjacent, anyOccupied>(longRangeCondition, shortRangeCondition, toParamaters(area));
}
void StockPileDestinationPathRequest::writeStep(Area& area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(path.empty() && !useCurrentLocation)
	{
		// Previously found path is no longer avaliable.
		actors.objective_canNotCompleteSubobjective(actorIndex);
		return;
	}
	m_objective.m_stockPileLocation = target;
	m_objective.m_hasCheckedForCloserDropOffLocation = true;
	// Destination found, join or create a project.
	assert(m_objective.m_stockPileLocation.exists());
	Space &space = area.getSpace();
	actors.canReserve_clearAll(actorIndex);
	const FactionId& actorFaction = actors.getFaction(actorIndex);
	assert(space.stockpile_getOneForFaction(m_objective.m_stockPileLocation, actorFaction) != nullptr);
	StockPile &stockpile = *space.stockpile_getOneForFaction(m_objective.m_stockPileLocation, actorFaction);
	if (stockpile.hasProjectNeedingMoreWorkers())
		stockpile.addToProjectNeedingMoreWorkers(actorIndex, m_objective);
	else
	{
		ItemIndex item = m_objective.getItem(area, actorIndex);
		if(item.empty())
		{
			// Found item no longer exists
			actors.objective_canNotCompleteSubobjective(actorIndex);
			return;
		}
		auto& hasStockPiles = area.m_hasStockPiles.getForFaction(actorFaction);
		Items& items = area.getItems();
		ItemReference itemRef = items.getReference(item);
		if (hasStockPiles.m_projectsByItem.contains(itemRef))
		{
			// Projects found, select one to join.
			for (StockPileProject* stockPileProject : hasStockPiles.m_projectsByItem[itemRef])
				if (stockPileProject->canAddWorker(actorIndex))
				{
					m_objective.m_project = stockPileProject;
					stockPileProject->addWorkerCandidate(actorIndex, m_objective);
					return;
				}
		}
		// No projects found, make one.
		// Pass responsability for tracking the item to the project.
		m_objective.m_itemStartLocation.clear();
		hasStockPiles.makeProject(item, m_objective.m_stockPileLocation, m_objective, actorIndex);
	}
}
StockPileDestinationPathRequest::StockPileDestinationPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json StockPileDestinationPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}