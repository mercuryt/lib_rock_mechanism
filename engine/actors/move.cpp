#include "actors.h"
#include "../area/area.h"
#include "../deserializationMemo.h"
#include "../fluidType.h"
#include "../definitions/moveType.h"
#include "../numericTypes/types.h"
#include "../simulation/simulation.h"
#include "../util.h"
#include "../path/pathRequest.h"
#include "../space/space.h"
#include "../portables.hpp"
#include "config.h"
#include "eventSchedule.h"
#include "items/items.h"
#include "path/terrainFacade.h"
#include <ranges>
Speed Actors::move_getIndividualSpeedWithAddedMass(const ActorIndex& index, const Mass& mass) const
{
	Speed output = Speed::create(m_agility[index].get() * Config::unitsOfMoveSpeedPerUnitOfAgility);
	Mass carryMass = m_equipmentSet[index]->getMass() + canPickUp_getMass(index) + onDeck_getMass(index) + mass;
	Mass unencomberedCarryMass = m_unencomberedCarryMass[index];
	if(carryMass > unencomberedCarryMass)
	{
		float ratio = (float)unencomberedCarryMass.get() / (float)carryMass.get();
		if(ratio < Config::minimumOverloadRatio)
			return Speed::create(0);
		output = Speed::create(std::ceil(output.get() * ratio * ratio));
	}
	output = Speed::create(util::scaleByInversePercent(output.get(), m_body[index]->getImpairMovePercent()));
	return Speed::create(std::ceil(output.get()));
}
void Actors::move_updateIndividualSpeed(const ActorIndex& index)
{
	m_speedIndividual[index] = move_getIndividualSpeedWithAddedMass(index, Mass::create(0));
	move_updateActualSpeed(index);
	// Ensure changes to mass are reflected in isOnDeckOf speed.
	if(m_isOnDeckOf[index].exists())
		m_isOnDeckOf[index].move_updateIndividualSpeed(m_area);
}
void Actors::move_updateActualSpeed(const ActorIndex& index)
{
	move_setMoveSpeedActual(index, isLeading(index) ? lead_getSpeed(index) : m_speedIndividual[index]);
}
void Actors::move_setPath(const ActorIndex& index, const SmallSet<Point3D>& path)
{
	assert(!path.empty());
	if(!m_isPilot[index])
		assert(m_location[index].isAdjacentTo(path.back()));
	m_path[index] = std::move(path);
	m_destination[index] = m_path[index].front();
	move_clearAllEventsAndTasks(index);
	move_schedule(index, m_location[index]);
}
void Actors::move_setType(const ActorIndex& index, const MoveTypeId& moveType)
{
	setMoveType(index, moveType);
	PathRequest* pathRequest = m_pathRequest[index];
	if(pathRequest != nullptr)
		pathRequest->moveType = moveType;
}
void Actors::move_setMoveSpeedActual(const ActorIndex& index, Speed speed)
{
	m_speedActual[index] = speed;
	// TODO: if there is a move event then reschedule it?
}
void Actors::move_clearPath(const ActorIndex& index)
{
	m_path[index].clear();
	move_clearAllEventsAndTasks(index);
}
void Actors::move_callback(const ActorIndex& index)
{
	assert(!m_path[index].empty());
	assert(m_destination[index].exists());
	const Point3D& newLocation = m_path[index].back();
	const Point3D previousLocation = getCombinedLocation(index);
	const Facing4 previousFacing = getFacing(index);
	Items& items = getItems();
	SetLocationAndFacingResult result;
	const Point3D& destination = move_getDestination(index);
	const Space& space = m_area.getSpace();
	const ShapeId& shape = getCompoundShape(index);
	const MoveTypeId& moveType = getMoveType(index);
	if(m_isPilot[index])
	{
		// If piloting then move the vehicle instead of the actor.
		// When piloting a mounted actor the mount should recive the move_callback so we assume that we are piloting a vehicle.
		const ItemIndex& isPiloting = m_isOnDeckOf[index].getItem();
		const auto itemAndResult = items.location_tryToMoveToDynamic(isPiloting, newLocation);
		result = itemAndResult.second;
	}
	else
		result = location_tryToMoveToDynamic(index, newLocation);
	if(isLeading(index) && result != SetLocationAndFacingResult::PermanantlyBlocked)
	{
		assert(!isFollowing(index));
		if(!lineLead_followersCanMoveEver(index))
			result = SetLocationAndFacingResult::PermanantlyBlocked;
		else if(result == SetLocationAndFacingResult::Success && !lineLead_followersCanMoveCurrently(index))
			result = SetLocationAndFacingResult::TemporarilyBlocked;
		// Roll back leader position if line cannot follow.
		if(result != SetLocationAndFacingResult::Success)
			location_set(index, previousLocation, previousFacing);
	}
	if(result == SetLocationAndFacingResult::PermanantlyBlocked)
	{
		// Path has become permanantly blocked since being generated.
		if(space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(destination, shape, moveType))
			// Destination is still valid.
			move_setDestination(index, destination);
		else
			// Destination no longer valid
			objective_canNotCompleteSubobjective(index);
		return;
	}
	if(result == SetLocationAndFacingResult::TemporarilyBlocked)
	{
		// Path is temporarily blocked, wait a bit and then detour if still blocked.
		if(m_moveRetries[index] == Config::moveTryAttemptsBeforeDetour)
		{
			if(m_hasObjectives[index]->hasCurrent())
				// Let the objective mediate the detor. Allows controll over reservations, finding a different adjacent to use as destination, etc.
				m_hasObjectives[index]->detour(m_area);
			else if(space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(destination, shape, moveType))
			{
				constexpr bool detour = true;
				// Destination is still valid, detour.
				move_setDestination(index, destination, detour);
			}
			else
				// Destination no longer valid
				objective_canNotCompleteSubobjective(index);
		}
		else
		{
			++m_moveRetries[index];
			move_schedule(index, m_location[index]);
		}
	}
	else
	{
		// Path is not blocked, move successful.
		m_moveRetries[index] = 0;
		// Move followers.
		if(isLeading(index))
		{
			lineLead_pushFront(index, newLocation);
			lineLead_moveFollowers(index);
		}
		// Respond to reaching end of path or attempt to schedule another move step.
		if(newLocation == destination)
		{
			m_path[index].clear();
			m_destination[index].clear();
			ActorIndex pilot = mount_getPilot(index);
			if(pilot.empty())
			{
				// If there is no rider piloting check if we are towing an item with a pilot.
				const ActorOrItemIndex& follower = getFollower(index);
				if(follower.exists() && follower.isItem())
					pilot = items.pilot_get(follower.getItem());
			}
			if(pilot.exists())
				m_hasObjectives[pilot]->subobjectiveComplete(m_area);
			else
				m_hasObjectives[index]->subobjectiveComplete(m_area);
		}
		else
		{
			assert(m_path[index].size() != 1);
			m_path[index].popBack();
			const Point3D& nextLocation = m_path[index].back();
			if(
				space.shape_anythingCanEnterEver(nextLocation) &&
				space.shape_shapeAndMoveTypeCanEnterEverFrom(nextLocation, shape, moveType, newLocation)
			)
				// Can take next step.
				move_schedule(index, newLocation);
			else if(space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(destination, shape, moveType))
				// Destination is still valid, repath.
				move_setDestination(index, destination);
			else
				// Destination no longer valid.
				objective_canNotCompleteSubobjective(index);
		}
	}
}
void Actors::move_schedule(const ActorIndex& index, const Point3D& moveFrom)
{
	assert(moveFrom != m_destination[index]);
	const Point3D& moveTo = m_path[index].back();
	m_moveEvent.schedule(index, move_delayToMoveInto(index, moveFrom, moveTo), m_area, index);
}
void Actors::move_setDestination(const ActorIndex& index, const Point3D& destination, bool detour, bool adjacent, bool unreserved, bool reserve)
{
	// location is either for the actor it's self or the vehicle it's piloting.
	Point3D location;
	if(m_isPilot[index])
	{
		auto& isOnDeckOf = m_isOnDeckOf[index];
		if(isOnDeckOf.isActor())
		{
			// When an actor is the pilot of their mount the move instructions get passed down to the mount.
			move_setDestination(isOnDeckOf.getActor(), destination, detour, adjacent, unreserved, reserve);
			return;
		}
		// Is piloting an item.
		Items& items = m_area.getItems();
		const ItemIndex& item = isOnDeckOf.getItem();
		const ActorOrItemIndex& leader = items.getLeader(item);
		if(leader.exists())
		{
			// Is piloting an item which is being towed by an animal, proxy setDestination to that animal.
			move_setDestination(leader.getActor(), destination, detour, adjacent, unreserved, reserve);
			return;
		}
		location = items.getLocation(item);
	}
	else
		location = getLocation(index);
	assert(destination.exists());
	if(reserve)
		assert(unreserved);
	move_clearPath(index);
	Space& space = m_area.getSpace();
	// If adjacent path then destination isn't known until it's completed.
	if(!adjacent)
	{
		m_destination[index] = destination;
		assert(location != m_destination[index]);
		if(!pilotItem_isPilotingConstructedItem(index))
			assert(location_canEnterEverWithAnyFacing(index, destination));
	}
	else
		assert(getLocation(index) != destination);
	move_clearAllEventsAndTasks(index);
	if(m_faction[index].empty())
		reserve = unreserved = false;
	if(unreserved && !adjacent)
		assert(!space.isReserved(destination, m_faction[index]));
	FactionId faction;
	if(unreserved)
		faction = m_faction[index];
	assert(m_pathRequest[index] == nullptr);
	const ShapeId& shape = getCompoundShape(index);
	const MoveTypeId& moveType = getMoveType(index);
	if(!adjacent)
		move_pathRequestRecord(index, std::make_unique<GoToPathRequest>(location, Distance::max(), getReference(index), shape, faction, moveType, m_facing[index], detour, adjacent, reserve, destination));
	else
	{
		SmallSet<Point3D> candidates;
		for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(destination))
			if(
				space.shape_anythingCanEnterEver(adjacent) &&
				space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(adjacent, shape, moveType)
			)
				candidates.insert(adjacent);
		move_pathRequestRecord(
			index,
			std::make_unique<GoToAnyPathRequest>(
				location, Distance::max(), getReference(index),
				shape, faction, moveType, m_facing[index],
				detour, adjacent, reserve, destination, candidates
			)
		);
	}
}
void Actors::move_setDestinationAdjacentToLocation(const ActorIndex& index, const Point3D& destination, bool detour, bool unreserved, bool reserve)
{
	move_setDestination(index, destination, detour, true, unreserved, reserve);
}
void Actors::move_setDestinationToAny(const ActorIndex& index, const SmallSet<Point3D>& candidates, bool detour, bool unreserved, bool reserve, const Point3D& huristicDestination)
{
	FactionId faction;
	if(unreserved)
		faction = m_faction[index];
	constexpr bool adjacent = false;
	move_pathRequestRecord(index, std::make_unique<GoToAnyPathRequest>(m_location[index], Distance::max(), getReference(index), m_compoundShape[index], faction, m_moveType[index], m_facing[index], detour, adjacent, reserve, huristicDestination, candidates));
}
void Actors::move_setDestinationAdjacentToActor(const ActorIndex& index, const ActorIndex& other, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToActor(index, other));
	const Point3D& otherLocation = m_location[other];
	assert(!isAdjacentToLocation(index, otherLocation));
	Space& space = m_area.getSpace();
	assert(m_pathRequest[index] == nullptr);
		SmallSet<Point3D> candidates;
	for(const Point3D& adjacent : getAdjacentPoints(other))
		if(
			space.shape_anythingCanEnterEver(adjacent) &&
			space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(adjacent, m_compoundShape[index], m_moveType[index])
		)
			candidates.insert(adjacent);
	move_setDestinationToAny(index, candidates, detour, unreserved, reserve, otherLocation);
}
void Actors::move_setDestinationAdjacentToItem(const ActorIndex& index, const ItemIndex& item, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToItem(index, item));
	assert(!isAdjacentToLocation(index, m_area.getItems().getLocation(item)));
	assert(m_pathRequest[index] == nullptr);
	Space& space = m_area.getSpace();
	Items& items = m_area.getItems();
	assert(m_pathRequest[index] == nullptr);
		SmallSet<Point3D> candidates;
		for(const Point3D& adjacent : items.getAdjacentPoints(item))
			if(
				space.shape_anythingCanEnterEver(adjacent) &&
				space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(adjacent, m_compoundShape[index], m_moveType[index])
			)
				candidates.insert(adjacent);
	move_setDestinationToAny(index, candidates, detour, unreserved, reserve, items.getLocation(item));
}
void Actors::move_setDestinationAdjacentToPlant(const ActorIndex& index, const PlantIndex& plant, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToPlant(index, plant));
	assert(m_pathRequest[index] == nullptr);
	Space& space = m_area.getSpace();
	Plants& plants = m_area.getPlants();
	assert(m_pathRequest[index] == nullptr);
		SmallSet<Point3D> candidates;
		for(const Point3D& adjacent : plants.getAdjacentPoints(plant))
			if(
				space.shape_anythingCanEnterEver(adjacent) &&
				space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(adjacent, m_compoundShape[index], m_moveType[index])
			)
				candidates.insert(adjacent);
	move_setDestinationToAny(index, candidates, detour, unreserved, reserve, plants.getLocation(plant));
}
void Actors::move_setDestinationAdjacentToPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex, bool detour, bool unreserved, bool reserve)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
		move_setDestinationAdjacentToActor(index, actorOrItemIndex.getActor(), detour, unreserved, reserve);
	else
		move_setDestinationAdjacentToItem(index, actorOrItemIndex.getItem(), detour, unreserved, reserve);
}
void Actors::move_setDestinationAdjacentToFluidType(const ActorIndex& index, const FluidTypeId& fluidType, bool detour, bool unreserved, bool reserve, Distance maxRange)
{
	assert(m_pathRequest[index] == nullptr);
	constexpr bool adjacent = true;
	FactionId faction;
	if(unreserved)
		faction = m_faction[index];
	move_pathRequestRecord(index, std::make_unique<GoToFluidTypePathRequest>(m_location[index], maxRange, getReference(index), m_compoundShape[index], faction, m_moveType[index], m_facing[index], detour, adjacent, reserve, fluidType));
}
void Actors::move_setDestinationAdjacentToDesignation(const ActorIndex& index, const SpaceDesignation& designation, bool detour, bool unreserved, bool reserve, Distance maxRange)
{
	assert(m_pathRequest[index] == nullptr);
	FactionId faction;
	if(unreserved)
		faction = m_faction[index];
	constexpr bool adjacent = true;
	move_pathRequestRecord(index, std::make_unique<GoToSpaceDesignationPathRequest>(m_location[index], maxRange, getReference(index), m_compoundShape[index], faction, m_moveType[index], m_facing[index], detour, adjacent, reserve, designation));
}
void Actors::move_setDestinationToEdge(const ActorIndex& index, bool detour)
{
	assert(m_pathRequest[index] == nullptr);
	constexpr bool adjacent = false;
	constexpr bool reserveDestination = false;
	move_pathRequestRecord(index, std::make_unique<GoToEdgePathRequest>(m_location[index], Distance::max(), getReference(index), m_compoundShape[index], m_moveType[index], m_facing[index], detour, adjacent, reserveDestination));
}
void Actors::move_clearAllEventsAndTasks(const ActorIndex& index)
{
	m_moveEvent.maybeUnschedule(index);
	move_pathRequestMaybeCancel(index);
}
void Actors::move_onLeaveArea(const ActorIndex& index) { move_clearAllEventsAndTasks(index); }
void Actors::move_onDeath(const ActorIndex& index) { move_clearAllEventsAndTasks(index); }
bool Actors::move_tryToReserveProposedDestination(const ActorIndex& index, const SmallSet<Point3D>& path)
{
	ShapeId shape = getShape(index);
	CanReserve& canReserve = canReserve_get(index);
	Space& space = m_area.getSpace();
	Point3D location = path.back();
	FactionId faction = getFaction(index);
	if(Shape::getIsMultiTile(shape))
	{
		assert(!path.empty());
		Point3D previous = path.size() == 1 ? getLocation(index) : path[path.size() - 2];
		Facing4 facing = previous.getFacingTwords(location);
		auto occupiedPoints = Shape::getPointsOccupiedAt(shape, space, location, facing);
		for(Point3D point : occupiedPoints)
			if(space.isReserved(point, faction))
				return false;
		for(Point3D point : occupiedPoints)
			space.reserve(point, canReserve);
	}
	else
	{
		if(space.isReserved(location, faction))
			return false;
		space.reserve(location, canReserve);
	}
	return true;
}
bool Actors::move_tryToReserveOccupied(const ActorIndex& index)
{
	ShapeId shape = getShape(index);
	CanReserve& canReserve = canReserve_get(index);
	Space& space = m_area.getSpace();
	Point3D location = getLocation(index);
	FactionId faction = getFaction(index);
	if(Shape::getIsMultiTile(shape))
	{
		assert(!m_path[index].empty());
		auto occupiedPoints = getOccupied(index);
		for(Point3D point : occupiedPoints)
			if(space.isReserved(point, faction))
				return false;
		for(Point3D point : occupiedPoints)
			space.reserve(point, canReserve);
	}
	else
	{
		if(space.isReserved(location, faction))
			return false;
		space.reserve(location, canReserve);
	}
	return true;
}
bool Actors::move_destinationIsAdjacentToLocation(const ActorIndex& index, const Point3D& location)
{
	assert(m_destination[index].exists());
	return location.isAdjacentTo(m_destination[index]);
}
void Actors::move_pathRequestCallback(const ActorIndex& index, SmallSet<Point3D> path, bool useCurrentLocation, bool reserveDestination)
{
	if(path.empty() || (reserveDestination && !move_tryToReserveProposedDestination(index, path)))
	{
		if(useCurrentLocation && (!reserveDestination || move_tryToReserveOccupied(index)))
		{
			// The actor's current location is already acceptable.
			m_hasObjectives[index]->subobjectiveComplete(m_area);
			return;
		}
		// TODO:
		// Should this be restart, or even cannotCompleteSubobjective?
		// cannotFulfill will cause a delay to be appiled before this objective can be added to the list again.
		// restart or cannotCompleteSubobjective might cause an infinite loop.
		// To prevent this we would have to know if we may reroute or not.
		// cannotFulfillObjective is the most pessamistic choice.
		// This is probably best solved by callback.
		HasObjectives& hasObjectives = *m_hasObjectives[index];
		//TODO: This conditional only exists because objectives are not mandated to always exist for all actors.
		if(hasObjectives.hasCurrent())
		{
			// Objectives can handle the path not found situation directly or default to cannotFulfill.
			Objective& objective = hasObjectives.getCurrent();
			if(!objective.onCanNotPath(m_area, index))
			{
				if(objective.isNeed())
					hasObjectives.cannotFulfillNeed(m_area, objective);
				else
					hasObjectives.cannotFulfillObjective(m_area, objective);
			}
		}
	}
	else
		move_setPath(index, path);
}
void Actors::move_pathRequestMaybeCancel(const ActorIndex& index)
{
	if(m_pathRequest[index] != nullptr)
	{
		m_pathRequest[index]->cancel(m_area);
		m_pathRequest[index] = nullptr;
	}
}
void Actors::move_pathRequestClear(const ActorIndex& index)
{
	assert(m_pathRequest[index]!= nullptr);
	m_pathRequest[index] = nullptr;
}
void Actors::move_pathRequestRecord(const ActorIndex& index, std::unique_ptr<PathRequestDepthFirst> pathRequest)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = pathRequest.get();
	m_area.m_hasTerrainFacades.getForMoveType(pathRequest->moveType).registerPathRequestWithHuristic(std::move(pathRequest));
}
void Actors::move_pathRequestRecord(const ActorIndex& index, std::unique_ptr<PathRequestBreadthFirst> pathRequest)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = pathRequest.get();
	m_area.m_hasTerrainFacades.getForMoveType(m_moveType[index]).registerPathRequestNoHuristic(std::move(pathRequest));
}
bool Actors::move_canMove(const ActorIndex& index) const
{
	if(!isAlive(index) || m_speedIndividual[index] == 0)
		return false;
	return true;
}
Step Actors::move_delayToMoveInto(const ActorIndex& index, const Point3D& from, const Point3D& to) const
{
	assert(from != to);
	Space& space = m_area.getSpace();
	//assert(from.isAdjacentTo(to));
	Speed speed = m_speedActual[index];
	CollisionVolume staticVolume = space.shape_getStaticVolume(to);
	if(staticVolume > Config::minPointStaticVolumeToSlowMovement)
	{
		CollisionVolume excessVolume = staticVolume - Config::minPointStaticVolumeToSlowMovement;
		if(excessVolume > 50)
			excessVolume = CollisionVolume::create(50);
		speed = Speed::create(util::scaleByPercent(speed.get(), Percent::create(100 - excessVolume.get())));
	}
	assert(speed != 0);
	static const Step stepsPerSecond = Config::stepsPerSecond;
	MoveCost cost = space.shape_moveCostFrom(to, m_moveType[index], from);
	assert(cost != 0);
	return Step::create(std::max(1u, uint(std::round(float(stepsPerSecond.get() * cost.get()) / float(speed.get())))));
}
SmallSet<Point3D> Actors::move_makePathTo(const ActorIndex& index, const Point3D& destination) const
{
	assert(destination != m_location[index]);
	const FindPathResult result = m_area.m_hasTerrainFacades.getForMoveType(
		m_moveType[index]).findPathToWithoutMemo(m_location[index], m_facing[index], m_compoundShape[index], destination
	);
	assert(!result.useCurrentPosition);
	return result.path;
}
Step Actors::move_stepsTillNextMoveEvent(const ActorIndex& index) const
{
	// Add 1 because we increment step number at the end of the step.
	return Step::create(1) + m_moveEvent.getStep(index) - m_area.m_simulation.m_step;
}
bool Actors::move_canPathTo(const ActorIndex& index, const Point3D& destination)
{
	return move_canPathFromTo(index, m_location[index], m_facing[index], destination);
}
bool Actors::move_canPathFromTo(const ActorIndex& index, const Point3D& start, const Facing4& startFacing, const Point3D& destination)
{
	TerrainFacade& terrainFacade = m_area.m_hasTerrainFacades.getForMoveType(m_moveType[index]);
	return terrainFacade.accessable(start, startFacing, destination, index);
}
// MoveEvent
MoveEvent::MoveEvent(const Step& delay, Area& area, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(actor) { }
MoveEvent::MoveEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>())
{
	data["actor"].get_to(m_actor);
}
void MoveEvent::execute(Simulation&, Area* area) { area->getActors().move_callback(m_actor); }
void MoveEvent::clearReferences(Simulation &, Area *area) { area->getActors().m_moveEvent.clearPointer(m_actor); }
Json MoveEvent::toJson() const
{
	Json output = {{"delay", duration()}, {"start", m_startStep}};
	output["actor"] = m_actor;
	return output;
}
