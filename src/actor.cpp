#include "actor.h"
#include "block.h"
#include "area.h"
#include "simulation.h"

#include <algorithm>

Actor::Actor(uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction& faction, Attributes attributes) :
	HasShape(species.shapeForPercentGrown(percentGrown), false), m_id(id), m_name(name), m_species(species), m_alive(true), m_awake(true), m_body(*this), m_project(nullptr), m_faction(&faction), m_attributes(attributes), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_needsSafeTemperature(*this), m_canMove(*this), m_canFight(*this), m_canPickup(*this), m_canGrow(*this, percentGrown), m_equipmentSet(*this), m_hasObjectives(*this), m_canReserve(faction), m_reservable(1) { }
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
}
void Actor::exit()
{
	assert(m_location != nullptr);
	m_location->m_hasActors.exit(*this);
}

void Actor::removeMassFromCorpse(uint32_t mass)
{
	assert(!m_alive);
	assert(mass <= m_attributes.getMass());
	m_attributes.removeMass(mass);
}
bool Actor::isEnemy(Actor& actor) const { return m_faction->enemies.contains(actor.m_faction); }
bool Actor::isAlly(Actor& actor) const { return m_faction->allies.contains(actor.m_faction); }
void Actor::die(CauseOfDeath causeOfDeath)
{
	m_causeOfDeath = causeOfDeath;
	m_canFight.onDie();
	m_location->m_area->m_hasActors.remove(*this);
}
void Actor::passout(uint32_t duration)
{
	//TODO
	(void)duration;
}
void Actor::doVision(const std::unordered_set<Actor*>& actors)
{
	(void)actors;
	//TODO: psycology.
	//TODO: fog of war?
}
uint32_t Actor::getMass() const
{
	return m_attributes.getMass() + m_equipmentSet.getMass() + m_canPickup.getMass();
}
uint32_t Actor::getVolume() const
{
	return m_body.getVolume();
}
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
ActorQuery ActorQuery::makeFor(Actor& a) { return ActorQuery(&a); }
ActorQuery ActorQuery::makeForCarryWeight(uint32_t cw) { return ActorQuery(cw, false, false); }

// To be used by block.
void BlockHasActors::enter(Actor& actor)
{
	assert(!contains(actor));
	if(actor.m_location != nullptr)
	{
		actor.m_location->m_hasActors.exit(actor);
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_location);
		m_block.m_area->m_hasActors.m_locationBuckets.update(actor, *actor.m_location, m_block);
	}
	else
		m_block.m_area->m_hasActors.m_locationBuckets.insert(actor);
	m_actors.push_back(&actor);
	m_block.m_hasShapes.enter(actor);
}
void BlockHasActors::exit(Actor& actor)
{
	assert(contains(actor));
	std::erase(m_actors, &actor);
	m_block.m_hasShapes.exit(actor);
}
void BlockHasActors::setTemperature(uint32_t temperature)
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
	m_locationBuckets.insert(actor);
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
void AreaHasActors::onChangeAmbiantSurfaceTemperature()
{
	for(Actor* actor : m_onSurface)
		if(!actor->m_location->m_underground)
			actor->m_needsSafeTemperature.onChange();
}
void AreaHasActors::processVisionReadStep()
{
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(simulation::step))
		m_visionRequestQueue.emplace_back(*actor);
	auto visionIter = m_visionRequestQueue.begin();
	while(visionIter < m_visionRequestQueue.end())
	{
		auto end = std::min(m_visionRequestQueue.end(), visionIter + Config::visionThreadingBatchSize);
		simulation::pool.push_task([=](){ VisionRequest::readSteps(visionIter, end); });
		visionIter = end;
	}
}
void AreaHasActors::processVisionWriteStep()
{
	for(VisionRequest& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
}
