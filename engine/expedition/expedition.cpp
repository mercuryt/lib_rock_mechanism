#include "expedition.h"
#include "transitZone.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../simulation/hasAreas.h"
#include "../definitions/moveType.h"
#include "../world/path.h"
#include "../objectives/joinExpedition.h"
void Expedition::updateMoveTypeAndSpeed(Simulation& simulation)
{
	SmallSet<MoveTypeId> moveTypes;
	Area& expeditionArea = simulation.m_hasAreas->getById(area);
	Actors& actorsRef = expeditionArea.getActors();
	Items& items = expeditionArea.getItems();
	moveSpeed = Speed::max();
	for(ActorIndex actor : actorsRef.getAll())
	{
		if(
			actorsRef.hasCarrier(actor) ||
			actorsRef.onDeck_isOnDeck(actor) ||
			actorsRef.isFollowing(actor)
		)
			continue;
		MoveTypeId actorMoveType = actorsRef.getMoveType(actor);
		moveTypes.maybeInsert(actorMoveType);
		moveSpeed = std::min(moveSpeed, actorsRef.move_getSpeed(actor));
	}
	for(ItemIndex item : items.getAll())
	{
		if(
			items.hasCarrier(item) ||
			items.onDeck_isOnDeck(item) ||
			items.isFollowing(item)
		)
			continue;
		MoveTypeId itemMoveType = items.getMoveType(item);
		moveTypes.maybeInsert(itemMoveType);
	}
	moveType = MoveType::reduce(moveTypes);
}
void Expedition::setExpeditionIdForAllActors(Simulation& simulation)
{
	auto& areaRef = simulation.m_hasAreas->getById(area);
	Actors& actorsRef = areaRef.getActors();
	for(ActorIndex actor : actorsRef.getAll())
		actorsRef.expedition_set(actor, id);
}
ActorIndex Expedition::createActor(Simulation& simulation, const ActorParamaters& params)
{
	Area& areaRef = simulation.m_hasAreas->getById(area);
	Actors& actorsRef = areaRef.getActors();
	ActorIndex index = actorsRef.create(params);
	ActorId actorId = actorsRef.getId(index);
	actors.insert(actorId);
	updateMoveTypeAndSpeed(simulation);
	return index;
}
void SimulationHasExpeditions::doStep(Simulation& simulation, Step step)
{
	for(ExpeditionId id : m_moveSchedule.doStep(step))
	{
		Expedition& expedition = byId(id);
		Point3D newLocation = expedition.path.back();
		expedition.path.pop_back();
		setLocation(expedition, simulation, newLocation);
		if(!expedition.path.empty())
			scheduleMove(simulation, expedition);
		else if(expedition.destinationArea == newLocation)
		{
			AreaId areaId = simulation.m_world->m_areas.queryGetOne(newLocation);
			if(areaId.empty() && expedition.enterDestination)
				areaId = simulation.m_world->createArea(simulation, newLocation);
			if(!areaId.empty())
			{
				Area& area = simulation.m_hasAreas->getById(areaId);
				m_arrivals.arrive(area, expedition);
			}
		}
	}
	m_arrivals.doStep(simulation, step);
}
void SimulationHasExpeditions::setLocation(Expedition& expedition, Simulation& simulation, Point3D location)
{
	assert(expedition.path.back() != location);
	auto& expeditionsTree = simulation.m_world->m_expeditions;
	expeditionsTree.remove(expedition.location, expedition.id);
	expeditionsTree.insert(location, expedition.id);
	expedition.location = location;
	// TODO: Update temperature?
}
void SimulationHasExpeditions::scheduleMove(Simulation& simulation, Expedition& expedition)
{
	MoveCost cost = Config::World::baseMoveCost;
	Distance newZ = expedition.path.back().z();
	int zDelta = newZ.get() - expedition.location.z().get();
	if(zDelta > 0)
		cost *= zDelta * Config::World::fractionToIncreaseCostByPerZLevelUp;
	if(zDelta < 0)
		cost *= zDelta * Config::World::fractionToDecreaseCostByPerZLevelDown;
	Step scheduledStep = simulation.m_step + (cost.get() / expedition.moveSpeed.get());
	m_moveSchedule.schedule(expedition.id, scheduledStep);
}
ExpeditionId SimulationHasExpeditions::create(Simulation& simulation, FactionId faction, Priority priority, Point3D location, ExpeditionType type, std::string name)
{
	// TODO: not sure about this.
	constexpr bool createDrama = false;
	Area& newArea = simulation.m_hasAreas->createArea(0,0,0, createDrama);
	ExpeditionId id = m_nextId++;
	m_expeditions.emplace_back(Expedition{.name=name, .location=location, .area=newArea.m_id, .faction=faction, .id=id, .priority=priority, .type=type});
	simulation.m_world->m_expeditions.insert(location, id);
	return m_expeditions.back().id;
}
void SimulationHasExpeditions::add(Area& area, ActorIndex actor, ExpeditionId expeditionId)
{
	Expedition& expedition = area.m_simulation.m_hasExpeditions.byId(expeditionId);
	Actors& actors = area.getActors();
	expedition.actors.insert(actors.getId(actor));
	expedition.waitingFor.insert(actors.getId(actor));
	actors.expedition_set(actor, expeditionId);
	std::unique_ptr<Objective> objective = std::make_unique<JoinExpeditionObjective>(expedition.priority, expedition.id);
	actors.objective_addTaskToEnd(actor, std::move(objective));
	expedition.updateMoveTypeAndSpeed(area.m_simulation);
}
void SimulationHasExpeditions::departAreaToJoinExpedition(Area& area, ActorIndex actor, Expedition& expedition)
{
	expeditionTransitZone::exitArea(expedition, area, ActorOrItemIndex::create(actor));
	if(expedition.waitingFor.empty() && !expedition.path.empty())
	{
		expedition.setExpeditionIdForAllActors(area.m_simulation);
		scheduleMove(area.m_simulation, expedition);
	}
}
void SimulationHasExpeditions::regroup(Simulation& simulation, Expedition& expedition)
{
	for(ActorId id : expedition.actors)
	{
		if(!expedition.waitingFor.contains(id))
			// This actor is already present.
			continue;
		auto& [actors, actor] = simulation.m_actors.getDataLocation(id);
		std::unique_ptr<Objective> objective = std::make_unique<JoinExpeditionObjective>(expedition.priority, expedition.id);
		actors->objective_addTaskToEnd(actor, std::move(objective));
	}
}
bool SimulationHasExpeditions::setDestination(Simulation& simulation, ExpeditionId id, Point3D location, bool enter)
{
	// TODO: Check for move cooldown and delay the expedition by as long as it has traveled while it "comes back".
	Expedition& expedition = byId(id);
	if(location == expedition.location && enter && expedition.waitingFor.empty())
	{
		// Expedition is already at location, but has not entered area, do so now.
		AreaId areaId = simulation.m_world->m_areas.queryGetOne(location);
		if(areaId.empty())
			// Area does not exist, create it.
			areaId = simulation.m_world->createArea(simulation, location);
		Area& area = simulation.m_hasAreas->getById(areaId);
		m_arrivals.arrive(area, expedition);
		return true;
	}
	expedition.destinationArea = location;
	const auto& enterable = getEnterable(*simulation.m_world, expedition.moveType);
	// TODO: Batch path requests by move type?
	expedition.path = findPathWorld(enterable, expedition.location, location);
	if(expedition.path.empty())
		return false;
	scheduleMove(simulation, expedition);
	expedition.enterDestination = enter;
	return true;
}
Step SimulationHasExpeditions::getNextStep() const
{
	if(m_moveSchedule.empty())
		return Step::max();
	return m_moveSchedule.getNextStep();
}
void SimulationHasExpeditions::clearDestination(ExpeditionId id)
{
	Expedition& expedition = byId(id);
	expedition.path.clear();
	expedition.nextMove.clear();
}
void SimulationHasExpeditions::remove(Simulation& simulation, ExpeditionId expeditionId, ActorId id)
{
	Expedition& expedition = byId(expeditionId);
	expedition.actors.erase(id);
	// Actor must not be with the main body of the expedition.
	// Actors which seperate while not in a permanant area must seperate into a new expedition.
	expedition.waitingFor.erase(id);
	if(expedition.waitingFor.empty() && !expedition.path.empty())
		scheduleMove(simulation, expedition);
}
Expedition& SimulationHasExpeditions::byId(ExpeditionId id)
{
	auto found = std::ranges::find_if(m_expeditions, [id](auto expedition){ return expedition.id == id; });
	return *found;
}
const Expedition& SimulationHasExpeditions::byId(ExpeditionId id) const
{
	return const_cast<SimulationHasExpeditions*>(this)->byId(id);
}
const RTreeBoolean& SimulationHasExpeditions::getEnterable(World& world, MoveTypeId moveType)
{
	auto found = m_enterables.find(moveType);
	if(found != m_enterables.end())
		return found->second;
	CuboidSet set = makeEnterableWorld(world, moveType, world.m_boundry);
	m_enterables.emplace(moveType, set);
	return m_enterables.back().second;
}
void ExpeditionsHaveScheduledMovements::schedule(ExpeditionId id, Step step)
{
// TODO: m_data is sorted so this could be binary search. Probably not worth it.
	auto found = std::ranges::find_if(m_data, [step](auto pair){ return pair.first == step; });
	if(found == m_data.end())
	{
		m_data.emplace_back(step, std::vector<ExpeditionId>());
		m_data.back().second.push_back(id);
		// Sort in descending order, next move step is m_data.back().
		std::ranges::sort(m_data, std::greater{}, [](auto pair) { return pair.first; });
	}
}
void ExpeditionsHaveScheduledMovements::unschedule(ExpeditionId id, Step step)
{
	auto foundStep = std::ranges::find_if(m_data, [step](auto pair){ return pair.first == step; });
	assert(foundStep != m_data.end());
	if(foundStep->second.size() == 1)
	{
		m_data.erase(foundStep);
		return;
	}
	auto foundId = std::ranges::find(foundStep->second, id);
	(*foundId) = foundStep->second.back();
	foundStep->second.pop_back();
}
std::vector<ExpeditionId> ExpeditionsHaveScheduledMovements::doStep(Step step)
{
	std::vector<ExpeditionId> output;
	if(m_data.back().first == step)
	{
		output = std::move(m_data.back().second);
		m_data.pop_back();
	}
	return output;
}
bool ExpeditionsHaveScheduledMovements::empty() const
{
	return m_data.empty();
}
Step ExpeditionsHaveScheduledMovements::getNextStep() const
{
	return m_data.front().first;
}
Point3D ExpeditionsHaveArrivals::findEntryPoint(Area& area, Expedition& expedition)
{
	Facing6 facingArrivingThrough = area.m_location.getFacing6Twords(expedition.location);
	Facing4 facing = expedition.location.getFacingTwords(area.m_location);
	CuboidSet candidates = expeditionTransitZone::makeTransitZone(area, facingArrivingThrough);
	const Actors& actors = area.getActors();
	const Items& items = area.getItems();
	SmallSet<ShapeId> shapes;
	for(ActorIndex actor : actors.getAll())
		shapes.maybeInsert(actors.getCompoundShape(actor));
	for(ItemIndex item : items.getAll())
		shapes.maybeInsert(items.getCompoundShape(item));
	Space& space = area.getSpace();
	for(Cuboid cuboid : candidates)
		for(Point3D candidate : cuboid)
		{
			bool pass = true;
			for(ShapeId shape : shapes)
			{
				Distance shiftForwardDistance = Shape::getDistanceFromBack(shape);
				// project candidate forward to where the entire shape is in bounds.
				Point3D shifted = Point3D::create(candidate.shift(facing, shiftForwardDistance));
				if(!space.shape_canFitEverOrCurrentlyDynamic(shifted, shape, facing, {}))
				{
					pass = false;
					break;
				}

			}
			if(pass)
				// All shapes fit, entry point found.
				return candidate;
		}
	// No entry point found.
	return {};
}
void ExpeditionsHaveArrivals::arrive(Area& arrivingAt, Expedition& expedition)
{
	Simulation& simulation = arrivingAt.m_simulation;
	Area& expeditionArea = simulation.m_hasAreas->getById(expedition.area);
	const Actors& actors = expeditionArea.getActors();
	int independentMoverCount{0};
	for(ActorIndex actor : actors.getAll())
		if(!(
			actors.isFollowing(actor) ||
			actors.onDeck_isOnDeck(actor)
		))
			++independentMoverCount;
	assert(independentMoverCount > 0);
	Point3D center = findEntryPoint(arrivingAt, expedition);
	m_data.emplace(center, expedition.id, arrivingAt.m_id);
	// Find center point.
}
void ExpeditionsHaveArrivals::doStep(Simulation& simulation, Step step)
{
	if(step % Config::expeditionArriveInterval == 0)
	{
		// Arrivals.
		auto arrivalsCopy = m_data;
		for(ArrivalData data : arrivalsCopy)
			doStepArrival(simulation.m_hasAreas->getById(data.area), simulation.m_hasExpeditions.byId(data.expedition), data.center);
		// No interval needed for departures: actors depart areas as soon as they touch the edge.
	}
}
void ExpeditionsHaveArrivals::doStepArrival(Area& arrivingAt, Expedition& expedition, Point3D center)
{
	Area& expeditionArea = arrivingAt.m_simulation.m_hasAreas->getById(expedition.area);
	const Actors& actors = expeditionArea.getActors();
	Actors& arrivingAtActors = arrivingAt.getActors();
	Facing6 facingArrivingThrough = arrivingAt.m_location.getFacing6Twords(expedition.location);
	Facing4 facing = expedition.location.getFacingTwords(arrivingAt.m_location);
	CuboidSet transitZone = expeditionTransitZone::makeTransitZone(arrivingAt, facingArrivingThrough);
	for(ActorIndex actor : actors.getAll())
	{
		if(
			actors.isFollowing(actor) ||
			actors.onDeck_isOnDeck(actor)
		)
			continue;
		// Also includes followers and etc.
		Point3D location = expeditionTransitZone::findArrivalPointInZone(arrivingAt, expeditionArea, facingArrivingThrough, actor, expedition.moveType, transitZone, center);
		if(location.empty())
			continue;
		ActorOrItemIndex actorOrMountOrVehicle = ActorOrItemIndex::create(actor);
		if(actors.mount_isPilot(actor))
		{
			// Pilots aren't placed explicitly, rather they are implicit with their mounts / vehicles along with other cargo / passengers.
			ActorOrItemIndex isBeingPiloted = actors.getIsPiloting(actor);
			// If the mount / vehicle is onDeck of another mount / vehicle it will be included implicitly with that one.
			if(!isBeingPiloted.isOnDeck(expeditionArea))
				actorOrMountOrVehicle = isBeingPiloted;
		}
		auto [newIndex, otherNewIndices] = expeditionTransitZone::arriveInZone(arrivingAt, expeditionArea, location, facing, actorOrMountOrVehicle);
		// Add to Expedition::waitingFor. They will be removed when they exit the area and rejoin the expedition.
		for(ActorOrItemIndex index : otherNewIndices)
			if(index.isActor())
			{
				ActorId id = arrivingAtActors.getId(index.getActor());
				expedition.waitingFor.insert(id);
			}
		// One per step at most.
		break;
	}
	if(actors.empty())
	{
		// Arrival complete.
		auto found = std::ranges::find(m_data.m_data, expedition.id, &ArrivalData::expedition);
		m_data.erase(found);
	}
}