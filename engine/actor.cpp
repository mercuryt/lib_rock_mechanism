#include "actor.h"
#include "animalSpecies.h"
#include "block.h"
#include "area.h"
#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "simulation.h"
#include "wait.h"

#include <algorithm>
#include <iostream>

Actor::Actor(Simulation& simulation, uint32_t id, const std::wstring& name, const AnimalSpecies& species, DateTime birthDate, Percent percentGrown, Faction* faction, Attributes attributes) :
	HasShape(simulation, species.shapeForPercentGrown(percentGrown), false), m_faction(faction), m_id(id), m_name(name), m_species(species), m_birthDate(birthDate), m_alive(true), m_body(*this), m_project(nullptr), m_attributes(attributes), m_equipmentSet(*this), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_needsSafeTemperature(*this), m_canPickup(*this), m_canMove(*this), m_canFight(*this), m_canGrow(*this, percentGrown), m_hasObjectives(*this), m_canReserve(faction), m_stamina(*this), m_hasUniform(*this), m_visionRange(species.visionRange)
{
	// TODO: Having this line here requires making the existance of objectives mandatory at all times. Good idea?
	//m_hasObjectives.getNext();
}
Actor::Actor(const Json& data, DeserializationMemo& deserializationMemo) :
	HasShape(data, deserializationMemo),
	m_faction(data.contains("faction") ? &deserializationMemo.m_simulation.m_hasFactions.byName(data["faction"].get<std::wstring>()) : nullptr), 
	m_id(data["id"].get<ActorId>()), m_name(data["name"].get<std::wstring>()), m_species(AnimalSpecies::byName(data["species"].get<std::string>())), 
	m_birthDate(data["birthDate"]), m_alive(data["alive"].get<bool>()), m_body(data["body"], deserializationMemo, *this), m_project(nullptr), 
	m_attributes(data["attributes"], *data["species"].get<const AnimalSpecies*>(), data["percentGrown"].get<Percent>()), 
	m_equipmentSet(data["equipmentSet"], *this), m_mustEat(data["mustEat"], *this), m_mustDrink(data["mustDrink"], *this), m_mustSleep(data["mustSleep"], *this), m_needsSafeTemperature(data["needsSafeTemperature"], *this), 
	m_canPickup(data["canPickup"], *this), m_canMove(data["canMove"], deserializationMemo, *this), m_canFight(data["canFight"], *this), m_canGrow(data["canGrow"], *this), 
	// Wait untill projects have been loaded before loading hasObjectives.
	m_hasObjectives(*this), m_canReserve(m_faction), m_stamina(*this, data["stamina"].get<uint32_t>()), m_hasUniform(*this), m_visionRange(m_species.visionRange)
{ 
	Block& block = deserializationMemo.blockReference(data["location"]);
	setLocation(block);
}
Json Actor::toJson() const
{
	Json data = HasShape::toJson();
	data["species"] = m_species;
	data["birthDate"] = m_birthDate;
	data["percentGrown"] = m_canGrow.growthPercent();
	data["id"] = m_id;
	data["name"] = m_name;
	data["alive"] = m_alive;
	data["body"] = m_body.toJson();
	data["attributes"] = m_attributes.toJson();
	data["equipmentSet"] = m_equipmentSet.toJson();
	data["mustEat"] = m_mustEat.toJson();
	data["mustDrink"] = m_mustDrink.toJson();
	data["mustSleep"] = m_mustSleep.toJson();
	data["canPickup"] = m_canPickup.toJson();
	data["canMove"] = m_canMove.toJson();
	data["canFight"] = m_canFight.toJson();
	data["canGrow"] = m_canGrow.toJson();
	data["stamina"] = m_stamina.get();
	if(m_faction != nullptr)
		data["faction"] = m_faction;
	if(m_location != nullptr)
		data["location"] = m_location;
	if(m_canReserve.hasReservations())
		data["canReserve"] = m_canReserve.toJson();
	if(m_hasUniform.exists())
		data["uniform"] = &m_hasUniform.get();
	return data;
}
void Actor::setLocation(Block& block)
{
	assert(&block != HasShape::m_location);
	assert(block.m_hasShapes.anythingCanEnterEver());
	if(m_location == nullptr)
	{
		assert(block.m_hasShapes.canEnterEverWithFacing(*this, m_facing));
		assert(block.m_hasShapes.canEnterCurrentlyWithFacing(*this, m_facing));
	}
	else
	{
		assert(block.m_hasShapes.canEnterEverFrom(*this, *m_location));
		assert(block.m_hasShapes.canEnterCurrentlyFrom(*this, *m_location));
	}
	block.m_hasActors.enter(*this);
	m_canLead.onMove();
}
void Actor::exit()
{
	assert(m_location != nullptr);
	m_location->m_hasActors.exit(*this);
}

void Actor::removeMassFromCorpse(Mass mass)
{
	assert(!m_alive);
	assert(mass <= m_attributes.getMass());
	m_attributes.removeMass(mass);
}
bool Actor::isEnemy(Actor& actor) const { return m_faction->enemies.contains(const_cast<Faction*>(actor.m_faction)); }
bool Actor::isAlly(Actor& actor) const { return m_faction->allies.contains(const_cast<Faction*>(actor.m_faction)); }
void Actor::die(CauseOfDeath causeOfDeath)
{
	m_causeOfDeath = causeOfDeath;
	m_alive = false;
	m_canFight.onDeath();
	m_canMove.onDeath();
	m_mustDrink.onDeath();
	m_mustEat.onDeath();
	m_mustSleep.onDeath();
	if(m_location != nullptr)
		m_location->m_area->m_hasActors.remove(*this);
	if(m_project != nullptr)
		m_project->removeWorker(*this);
	setStatic(true);
}
void Actor::passout(Step duration)
{
	//TODO
	(void)duration;
}
void Actor::doVision(std::unordered_set<Actor*>& actors)
{
	m_canSee.swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void Actor::leaveArea()
{
	m_canFight.onLeaveArea();
	m_canMove.onLeaveArea();
	Area& area = *m_location->m_area;
	// Run remove before exit becuse we need to use location to know which bucket to remove from.
	area.m_hasActors.remove(*this);
	exit();
}
void Actor::wait(Step duration)
{
	m_hasObjectives.addTaskToStart(std::make_unique<WaitObjective>(*this, duration));
}
void Actor::takeHit(Hit& hit, BodyPart& bodyPart)
{
	m_equipmentSet.modifyImpact(hit, bodyPart.bodyPartType);
	m_body.getHitDepth(hit, bodyPart);
	if(hit.depth != 0)
		m_body.addWound(bodyPart, hit);
}
Mass Actor::getMass() const
{
	return m_attributes.getMass() + m_equipmentSet.getMass() + m_canPickup.getMass();
}
Volume Actor::getVolume() const
{
	return m_body.getVolume();
}
uint32_t Actor::getAgeInYears() const
{
	DateTime now = const_cast<Actor&>(*this).getSimulation().m_now;
	uint32_t differenceYears = now.year - m_birthDate.year;
	if(now.day > m_birthDate.day)
		++differenceYears;
	return differenceYears;
}
bool Actor::allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing) const
{
	if(m_faction == nullptr)
		return true;
	return HasShape::allBlocksAtLocationAndFacingAreReservable(location, facing, *m_faction);
}
void Actor::reserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing)
{
	if(m_faction == nullptr)
		return;
	for(Block* occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
		occupied->m_reservable.reserveFor(m_canReserve, 1u);
}
void Actor::unreserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing)
{
	if(m_faction == nullptr)
		return;
	for(Block* occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
		occupied->m_reservable.clearReservationFor(m_canReserve);
}
EventSchedule& Actor::getEventSchedule() { return getSimulation().m_eventSchedule; }
ThreadedTaskEngine& Actor::getThreadedTaskEngine() { return getSimulation().m_threadedTaskEngine; }
void Actor::log() const
{
	std::wcout << m_name;
	std::cout << "(" << m_species.name << ")";
	HasShape::log();
	std::cout << std::endl;
}
// To be used by block.
void BlockHasActors::enter(Actor& actor)
{
	assert(!contains(actor));
	if(actor.m_location != nullptr)
	{
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_location);
		m_block.m_area->m_hasActors.m_locationBuckets.update(actor, *actor.m_location, m_block);
		actor.m_location->m_hasActors.exit(actor);
	}
	else
		m_block.m_area->m_hasActors.m_locationBuckets.insert(actor, m_block);
	m_actors.push_back(&actor);
	m_block.m_hasShapes.enter(actor);
	if(m_block.m_underground)
		m_block.m_area->m_hasActors.setUnderground(actor);
	else
		m_block.m_area->m_hasActors.setNotUnderground(actor);
}
void BlockHasActors::exit(Actor& actor)
{
	assert(contains(actor));
	std::erase(m_actors, &actor);
	m_block.m_hasShapes.exit(actor);
}
void BlockHasActors::setTemperature(Temperature temperature)
{
	(void)temperature;
	for(Actor* actor : m_actors)
		actor->m_needsSafeTemperature.onChange();
}
bool BlockHasActors::contains(Actor& actor) const
{
	return std::ranges::find(m_actors, &actor) != m_actors.end();
}
void AreaHasActors::add(Actor& actor)
{
	assert(actor.m_location != nullptr);
	assert(!m_actors.contains(&actor));
	m_actors.insert(&actor);
	m_locationBuckets.insert(actor, *actor.m_location);
	m_visionBuckets.add(actor);
	if(!actor.m_location->m_underground)
		m_onSurface.insert(&actor);
}
void AreaHasActors::remove(Actor& actor)
{
	m_actors.erase(&actor);
	m_locationBuckets.erase(actor);
	m_visionBuckets.remove(actor);
	m_onSurface.erase(&actor);
}
void AreaHasActors::processVisionReadStep()
{
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(m_area.m_simulation.m_step))
		m_visionRequestQueue.emplace_back(*actor);
	auto visionIter = m_visionRequestQueue.begin();
	while(visionIter < m_visionRequestQueue.end())
	{
		auto end = std::min(m_visionRequestQueue.end(), visionIter + Config::visionThreadingBatchSize);
		m_area.m_simulation.m_taskFutures.push_back(m_area.m_simulation.m_pool.submit([=](){ VisionRequest::readSteps(visionIter, end); }));
		visionIter = end;
	}
}
void AreaHasActors::processVisionWriteStep()
{
	for(VisionRequest& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
}
void AreaHasActors::onChangeAmbiantSurfaceTemperature()
{
	for(Actor* actor : m_onSurface)
		actor->m_needsSafeTemperature.onChange();
}
void AreaHasActors::setUnderground(Actor& actor)
{
	m_onSurface.erase(&actor);
	actor.m_needsSafeTemperature.onChange();
}
void AreaHasActors::setNotUnderground(Actor& actor)
{
	m_onSurface.insert(&actor);
	actor.m_needsSafeTemperature.onChange();
}