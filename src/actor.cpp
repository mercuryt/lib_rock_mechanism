#include "actor.h"

Actor::Actor(Block& l, const Shape& s, const MoveType& mt, uint32_t ms, uint32_t m, Faction* f) : 
		HasShape<Block>(&s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(&mt), m_moveSpeed(ms), m_taskDelayCount(0), m_mass(m), m_alive(true), m_actionEvent(nullptr), m_objectiveQueue(ObjectiveSort), m_faction(f)
	{
		setLocation(l);
		setCombatScore();
	}
Actor::Actor(const Shape& s, const MoveType& mt, uint32_t ms, uint32_t m, Faction* f) : 
		HasShape<Block>(&s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(&mt), m_moveSpeed(ms), m_taskDelayCount(0),  m_mass(m), m_alive(true), m_faction(f)
	{
		setCombatScore();
	}
void Actor::setLocation(Block& block)
	{
		assert(block != *HasShape<Block>::m_location);
		assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(getShape(), *m_moveType));
		assert(block.actorCanEnterCurrently(derived()));
		block.m_hasActors.enter(derived());
	}
bool Actor::isEnemy(Actor& actor) { return m_faction.m_enemies.contains(actor.m_faction); }
bool Actor::isAlly(Actor& actor) { return m_faction.m_allies.contains(actor.m_faction); }
void Actor::die()
{
	m_canFight.noLongerTargetable();
	m_locaton->m_area.unregisterActor(*this);
}
static Actor create(Faction& faction, const AnimalSpecies& species, std::vector<AttributeModifiers>& modifiers, uint32_t percentGrown)
Actor::s_nextId = 1;

// ActorQuery, to be used to search for actors.
bool ActorQuery::operator()(Actor& actor) const
{
	if(actor != nullptr)
		return actor == &actor;
	if(carryWeight != 0 && actor.m_hasItems.getCarryWeight() < carryWeight)
		return false;
	if(checkIfSentient && m_actor.isSentient() != sentient)
		return false;
	return true;
}
static ActorQuery ActorQuery::makeFor(Actor& a) { return ActorQuery(a, 0, false, false); }
static ActorQuery ActorQuery::makeForCarryWeight(uint32_t cw) { return ActorQuery(nullptr, cw, false, false); }

// To be used by block.
HasActors::HasActors(Area& a) : m_area(a), m_volume(0) { }
void HasActors::enter(Actor& actor)
{
	assert(actor.m_location != &m_block);
	assert(canAdd(actor));
	assert(std::ranges::find(m_actors, &actor) == m_actors.end());
	const uint8_t facing;
	if(actor.m_locaton != nullptr)
	{
		actor.m_locaton->m_hasActors.exit(actor);
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_locaton);
		m_area->m_locationBuckets.update(actor, *actor.m_location, m_block);
	}
	else
	{
		actor.m_facing = 0;
		m_area->m_locationBuckets.insert(actor);
	}
	actor.m_location = &m_block;
	for(auto& [m_x, m_y, m_z, v] : item.itemType.shape.positionsWithFacing(actor.m_facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		assert(block != nullptr);
		assert(!block->isSolid());
		assert(std::ranges::find(block->m_actors, &actor) == m_actors.end());
		block->m_hasActors.volume += v;
		block->m_hasActors.m_actors.push_back(&item);
		actor.m_blocks.push_back(block);
	}
}
void HasActors::exit(Actor& actor)
{
	assert(actor.m_location == m_block);
	assert(canEnter(actor));
	assert(std::ranges::find(m_actors, &actor) != m_actors.end());
	for(auto& [m_x, m_y, m_z, v] : item.itemType.shape.positionsWithFacing(actor.m_facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		assert(block != nullptr);
		assert(!block->isSolid());
		assert(std::ranges::find(block->m_actors, &actor) != m_actors.end());
		block->m_hasActors.volume -= v;
		std::erase(block->m_hasActors.m_actors, &actor);
	}
	actor.m_locaton = nullptr;
	actor.m_blocks.clear();
}
bool HasActors::canEnterEverFrom(Actor& actor, Block& block) const
{
	if(m_block.isSolid())
		return false;
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(block);
	for(auto& [m_x, m_y, m_z, v] : actor.shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		if(block == nullptr)
			return false;
		if(block->isSolid())
			return false;
		if(!Config::moveTypeCanEnter(&block, actor.m_moveType))
			return false;
	}
	return true;
}
bool HasActors::canEnterCurrentlyFrom(Actor& actor, Block& block) const
{
	assert(canEnterEverFrom(actor));
	if(shape.positions.size() == 1)
	{
		const DerivedBlock& block = const_derived();
		uint32_t v = shape.positions[0][3];
		if(&block == actor.m_location)
			v = 0;
		return block.m_totalDynamicVolume + v <= Config::maxBlockVolume;
	}
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(block);
	for(auto& [m_x, m_y, m_z, v] : actor.shape.positionsWithFacing(facing))
	{
		const Block* block = m_block.offset(m_x, m_y, m_z);
		auto found = block->m_actors.find(&actor);
		if(found != block->m_actors.end())
		{
			if(block->m_totalDynamicVolume + v - found->second > Config::maxBlockVolume)
				return false;
		}
		else if(block->m_totalDynamicVolume + v > Config::maxBlockVolume)
			return false;
	}
	return true;
}
std::vector<Block*, uint32_t> HasActors::getMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	std::vector<Block*, uint32_t> output;
	for(Block* block : m_block.m_adjacentsVector)
		if(Config::moveTypeCanEnterEverFromTo(moveType, m_block, block))
			output.emplace_back(block, Config::getMoveCostFromTo(moveType, m_block, block));
	return output;
}
void HasActors::clearCache()
{
	m_moveCostsCache.clear();
	for(Block* block : getAdjacentWithEdgeAndCornerAdjacent())
		block->m_hasActors.m_moveCostsCache.clear();
}
