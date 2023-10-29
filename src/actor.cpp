#include "actor.h"
#include "block.h"
#include "area.h"
#include "simulation.h"
#include "wait.h"

#include <algorithm>

Actor::Actor(Simulation& simulation, uint32_t id, const std::wstring& name, const AnimalSpecies& species, Percent percentGrown, Faction* faction, Attributes attributes) :
	HasShape(simulation, species.shapeForPercentGrown(percentGrown), false), m_faction(faction), m_id(id), m_name(name), m_species(species), m_alive(true), m_body(*this), m_project(nullptr), m_attributes(attributes), m_equipmentSet(*this), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_needsSafeTemperature(*this), m_canPickup(*this), m_canMove(*this), m_canFight(*this), m_canGrow(*this, percentGrown), m_hasObjectives(*this), m_canReserve(faction), m_stamina(*this), m_visionRange(species.visionRange) 
{
	// TODO: Having this line here requires making the existance of objectives mandatory at all times. Good idea?
	//m_hasObjectives.getNext();
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
// ActorQuery, to be used to search for actors.
bool ActorQuery::operator()(Actor& other) const
{
	if(actor != nullptr)
		return actor == &other;
	if(carryWeight != 0 && other.m_canPickup.getMass() < carryWeight)
		return false;
	if(checkIfSentient && other.isSentient() != sentient)
		return false;
	return true;
}
ActorQuery ActorQuery::makeFor(Actor& a) { return ActorQuery(a); }
ActorQuery ActorQuery::makeForCarryWeight(Mass cw) { return ActorQuery(cw, false, false); }

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
		m_area.m_simulation.m_pool.push_task([=](){ VisionRequest::readSteps(visionIter, end); });
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
