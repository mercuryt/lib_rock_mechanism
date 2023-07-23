#include "actor.h"
#include "block.h"
#include "area.h"

#include <algorithm>

Actor::Actor(uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction& faction, Attributes attributes) :
	HasShape(species.shapeForPercentGrown(percentGrown), false), m_id(id), m_name(name), m_species(species), m_alive(true), m_awake(true), m_body(*this) , m_faction(&faction), m_attributes(attributes), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_needsSafeTemperature(*this), m_canMove(*this), m_canFight(*this), m_canPickup(*this), m_canGrow(*this, percentGrown), m_hasObjectives(*this), m_reservable(1) { }
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
bool Actor::isEnemy(Actor& actor) const { return m_faction->enemies.contains(actor.m_faction); }
bool Actor::isAlly(Actor& actor) const { return m_faction->allies.contains(actor.m_faction); }
void Actor::die(CauseOfDeath causeOfDeath)
{
	m_causeOfDeath = causeOfDeath;
	m_canFight.onDie();
	m_location->m_area->unregisterActor(*this);
}
void Actor::passout(uint32_t duration)
{
	//TODO
	(void)duration;
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
ActorQuery ActorQuery::makeFor(Actor& a) { return ActorQuery(&a, 0, false, false); }
ActorQuery ActorQuery::makeForCarryWeight(uint32_t cw) { return ActorQuery(nullptr, cw, false, false); }

// To be used by block.
void HasActors::enter(Actor& actor)
{
	assert(!contains(actor));
	if(actor.m_location != nullptr)
	{
		actor.m_location->m_hasActors.exit(actor);
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_location);
		m_block.m_area->m_actorLocationBuckets.update(actor, *actor.m_location, m_block);
	}
	else
		m_block.m_area->m_actorLocationBuckets.insert(actor);
	m_actors.push_back(&actor);
	m_block.m_hasShapes.enter(actor);
}
void HasActors::exit(Actor& actor)
{
	assert(contains(actor));
	std::erase(m_actors, &actor);
	m_block.m_hasShapes.exit(actor);
}
bool HasActors::contains(Actor& actor) const
{
	return std::ranges::find(m_actors, &actor) != m_actors.end();
}
