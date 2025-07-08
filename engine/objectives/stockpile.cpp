#include "stockpile.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../space/space.h"
#include "../path/terrainFacade.hpp"
#include "../numericTypes/types.h"
#include "../hasShapes.hpp"
// Objective Type.
bool StockPileObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot stockpile.
	// TODO: allow pilots?
	if(actors.mount_exists(actor))
		return false;
	return area.m_hasStockPiles.getForFaction(actors.getFaction(actor)).isAnyHaulingAvailableFor(actor);
}
std::unique_ptr<Objective> StockPileObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	return std::make_unique<StockPileObjective>();
}
// Objective.
StockPileObjective::StockPileObjective() : Objective(Config::stockPilePriority) { }
StockPileObjective::StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Objective(data, deserializationMemo),
	m_project(data.contains("project") ? static_cast<StockPileProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{
	if(data.contains("item"))
		m_item.load(data["item"], area.getItems().m_referenceData);
	if(data.contains("stockPileLocation"))
		m_stockPileLocation = data["stockPileLocation"].get<Point3D>();
	if(data.contains("hasCheckedForDropOffLocation"))
		m_hasCheckedForCloserDropOffLocation = true;
	if(data.contains("pickUpFacing"))
	{
		m_pickUpFacing = data["pickUpFacing"].get<Facing4>();
		m_pickUpLocation = data["pickUpLocation"].get<Point3D>();
	}
}
Json StockPileObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = *m_project;
	if(m_item.exists())
		data["item"] = m_item;
	if(m_stockPileLocation.exists())
		data["stockPileLocation"] = m_stockPileLocation;
	if(m_hasCheckedForCloserDropOffLocation)
		data["hasCheckedForDropOffLocation"] = true;
	if(m_pickUpFacing != Facing4::Null)
	{
		assert(m_pickUpLocation.exists());
		data["m_pickUpFacing"] = m_pickUpFacing;
		data["m_pickUpLocation"] = m_pickUpLocation;
	}
	return data;
}
void StockPileObjective::execute(Area& area, const ActorIndex& actor)
{
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
	{
		Actors& actors = area.getActors();
		assert(!actors.project_exists(actor));
		if(m_item.empty())
			actors.move_pathRequestRecord(actor, std::make_unique<StockPilePathRequest>(area, *this, actor));
		else
			actors.move_pathRequestRecord(actor, std::make_unique<StockPileDestinationPathRequest>(area, *this, actor));
	}
	else
		m_project->commandWorker(actor);
}
void StockPileObjective::cancel(Area& area, const ActorIndex& actor)
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
void StockPileObjective::reset(Area& area, const ActorIndex& actor)
{
	m_hasCheckedForCloserDropOffLocation = false;
	m_item.clear();
	m_stockPileLocation.clear();
	if constexpr(DEBUG)
	{
		m_pickUpFacing = Facing4::Null;
		m_pickUpLocation.clear();
	}
	cancel(area, actor);
}
bool StockPileObjective::destinationCondition(Area& area, const Point3D& point, const ItemIndex& item, const ActorIndex& actor)
{
	Space& space =  area.getSpace();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	if(!space.item_empty(point))
		// Don't put multiple items in the same point unless they are generic and share item type and material type.
		if(!items.isGeneric(item) || space.item_getAll(point).size() != 1 || space.item_getCount(point, items.getItemType(item), items.getMaterialType(item)) == 0)
			return false;
	FactionId faction = actors.getFaction(actor);
	const StockPile* stockpile = space.stockpile_getForFaction(point, faction);
	if(stockpile == nullptr || !stockpile->isEnabled() || !space.stockpile_isAvalible(point, faction))
		return false;
	if(space.isReserved(point, faction))
		return false;
	if(!stockpile->accepts(item))
		return false;
	m_stockPileLocation = point;
	return true;
};
// Path Reqests
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
StockPilePathRequest::StockPilePathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actorIndex) :
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
	reserveDestination = true;
}
FindPathResult StockPilePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	Space& space =  area.getSpace();
	Items& items = area.getItems();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	assert(!actors.project_exists(actorIndex));
	const FactionId& faction = actors.getFaction(actorIndex);
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	AreaHasSpaceDesignationsForFaction& designationsForFaction = area.m_spaceDesignations.getForFaction(faction);
	auto predicate = [this, actorIndex, faction, &hasStockPiles, &space, &items, &area, &designationsForFaction](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D>
	{
		if(designationsForFaction.check(point, SpaceDesignation::StockPileHaulFrom)) [[unlikely]]
		{
			for(ItemIndex item : space.item_getAll(point))
			{
				// Multi point items could be encountered more then once.
				if(m_items.contains(item))
					continue;
				if(items.reservable_isFullyReserved(item, faction))
					// Cannot stockpile reserved items.
					continue;
				if(items.stockpile_canBeStockPiled(item, faction))
				{
					// Item can be stockpiled, check if any of the stockpiles we have seen so far will accept it.
					for (const auto& [stockpile, space] : m_pointsByStockPile)
						for(const Point3D& point : space)
							if (stockpile->accepts(item) && checkDestination(area, item, point))
							{
								// Success
								m_objective.m_item = area.getItems().m_referenceData.getReference(item);
								m_objective.m_stockPileLocation = point;
								return {true, point};
							}
					// Item does not match any stockpile seen so far, store it to compare against future stockpiles.
					m_items.insert(item);
				}
			}
		}
		if(designationsForFaction.check(point, SpaceDesignation::StockPileHaulTo)) [[unlikely]]
		{
			StockPile *stockPile = space.stockpile_getForFaction(point, faction);
			if(stockPile != nullptr && (!m_pointsByStockPile.contains(stockPile) || !m_pointsByStockPile[stockPile].contains(point)))
			{
				if(space.isReserved(point, faction))
					return {false, point};
				if(!space.item_empty(point))
				{
					// Point static volume is full, don't stockpile here.
					if(space.shape_getStaticVolume(point) >= Config::maxPointVolume)
						return {false, point};
					// There is more then one item type in this point, don't stockpile anything here.
					if(space.item_getAll(point).size() != 1)
						return {false, point};
					// Non generics don't stack, don't stockpile anything here if there is a non generic present.
					ItemTypeId itemType = items.getItemType(space.item_getAll(point).front());
					if(!ItemType::getIsGeneric(itemType))
						return {false, point};
				}
				for(ItemIndex item : m_items)
					if(stockPile->accepts(item) && checkDestination(area, item, point))
					{
						// Success
						m_objective.m_item = area.getItems().m_referenceData.getReference(item);
						m_objective.m_stockPileLocation = point;
						return {true, point};
					}
				// No item seen so far matches the found stockpile, store it to check against future items.
				m_pointsByStockPile.getOrCreate(stockPile).insert(point);
			}
		}
		return {false, point};
	};
	//TODO: Optimization oppurtunity: path is unused.
	constexpr bool anyOccupied = true;
	// TODO: Add a SpaceDesignation to the query?
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupied, decltype(predicate)>(predicate, memo, start, facing, shape, detour, adjacent, faction, maxRange);
}
void StockPilePathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	const FactionId& faction = actors.getFaction(actorIndex);
	if(!m_objective.m_item.exists())
	{
		// No haulable item found.
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
	}
	else
	{
		assert(m_objective.m_stockPileLocation.exists());
		Items& items = area.getItems();
		const ItemIndex& item = m_objective.m_item.getIndex(items.m_referenceData);
		if(
			items.reservable_isFullyReserved(item, faction) ||
			!m_objective.destinationCondition(area, m_objective.m_stockPileLocation, item, actorIndex)
		)
		{
			// Found destination is no longer valid or item has been reserved
			m_objective.m_item.clear();
			m_objective.m_stockPileLocation.clear();
			m_objective.execute(area, actorIndex);
		}
		else
		{
			// Haulable Item and potential destination found. Call StockPileObjective::execute to create the destination path request in order to try and find a better destination.
			m_objective.m_pickUpLocation = result.path.back();
			if(result.useCurrentPosition)
				m_objective.m_pickUpFacing = actors.getFacing(actorIndex);
			else
			{
				const Point3D &secondToLast = result.path.size() > 1 ? *(result.path.end() - 2) : actors.getLocation(actorIndex);
				m_objective.m_pickUpFacing = secondToLast.getFacingTwords(result.path.back());
			}
			m_objective.execute(area, actorIndex);
		}
	}
}
bool StockPilePathRequest::checkDestination(const Area& area, const ItemIndex& item, const Point3D& point) const
{
	const Space& space =  area.getSpace();
	const Items& items = area.getItems();
	// Only stockpile in non empty point if it means stacking generics.
	if(!space.item_empty(point))
	{
		const ItemTypeId& itemType = items.getItemType(item);
		if(!ItemType::getIsGeneric(itemType))
			return false;
		// item cannot stack with the item in this point, don't stockpile it here.
		if(space.item_getCount(point, itemType, items.getMaterialType(item)) == 0)
			return false;
	}
	const ShapeId& singleQuantityShape = ItemType::getShape(items.getItemType(item));
	return
		space.shape_canEnterEverOrCurrentlyWithAnyFacingReturnFacingStatic(point, singleQuantityShape, items.getMoveType(item), items.getOccupied(item)) !=
		Facing4::Null;
}
StockPilePathRequest::StockPilePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json StockPilePathRequest::toJson() const
{
	Json output = static_cast<const PathRequestBreadthFirst&>(*this);
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "stockpile";
	return output;
}
StockPileDestinationPathRequest::StockPileDestinationPathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actorIndex) :
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
FindPathResult StockPileDestinationPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	Space& space =  area.getSpace();
	Items& items = area.getItems();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	assert(!actors.project_exists(actorIndex));
	const FactionId& faction = actors.getFaction(actorIndex);
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	auto predicate = [this, actorIndex, faction, &hasStockPiles, &space, &items, &area](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D>{
		return {m_objective.destinationCondition(area, point, m_objective.m_item.getIndex(items.m_referenceData), actorIndex), point};
	};
	constexpr bool anyOccupied = true;
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupied, decltype(predicate)>(predicate, memo, start, facing, shape, detour, adjacent, faction, maxRange);
}
void StockPileDestinationPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
	{
		// Previously found path is no longer avaliable.
		actors.objective_canNotCompleteSubobjective(actorIndex);
		return;
	}
	m_objective.m_stockPileLocation = result.pointThatPassedPredicate;
	m_objective.m_hasCheckedForCloserDropOffLocation = true;
	// Destination found, join or create a project.
	assert(m_objective.m_stockPileLocation.exists());
	Space &space =  area.getSpace();
	const ActorIndex& actor = actorIndex;
	actors.canReserve_clearAll(actor);
	const FactionId& faction = actors.getFaction(actor);
	assert(space.stockpile_getForFaction(m_objective.m_stockPileLocation, faction));
	StockPile &stockpile = *space.stockpile_getForFaction(m_objective.m_stockPileLocation, faction);
	if (stockpile.hasProjectNeedingMoreWorkers())
		stockpile.addToProjectNeedingMoreWorkers(actor, m_objective);
	else
	{
		auto &hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
		if (hasStockPiles.m_projectsByItem.contains(m_objective.m_item))
		{
			// Projects found, select one to join.
			for (StockPileProject* stockPileProject : hasStockPiles.m_projectsByItem[m_objective.m_item])
				if (stockPileProject->canAddWorker(actor))
				{
					m_objective.m_project = stockPileProject;
					stockPileProject->addWorkerCandidate(actor, m_objective);
					return;
				}
		}
		// No projects found, make one.
		Items& items = area.getItems();
		ItemIndex item = m_objective.m_item.getIndex(items.m_referenceData);
		// Pass responsability for tracking the item to the project.
		m_objective.m_item.clear();
		hasStockPiles.makeProject(item, m_objective.m_stockPileLocation, m_objective, actor);
	}
}
StockPileDestinationPathRequest::StockPileDestinationPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json StockPileDestinationPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}