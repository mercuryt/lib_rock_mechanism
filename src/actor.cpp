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
bool HasActors::anyoneCanEnterEver() const
{
	if(m_block.isSolid())
		return false;
	for(const BlockFeature& blockFeature : m_features)
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::fortification || blockFeature.blockFeatureType == &BlockFeatureType::floodGate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::door && blockFeature.locked)
		  )
			return false;
	}
	return true;
}
bool HasActors::canEnterEverFrom(Actor& actor, Block& block) const
{
	return shapeAndMoveTypeCanEnterEverFrom(actor.m_shape, actor.m_moveType, block);
}
bool HasActors::shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, Block& from) const
{
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(from);
	for(auto& [m_x, m_y, m_z, v] : actor.shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		if(block == nullptr)
			return false;
		if(block->isSolid())
			return false;
		if(!block->moveTypeCanEnter(moveType))
			return false;
	}
	return true;
}
bool HasActors::moveTypeCanEnter(const MoveType& moveType) const
{
	// Swiming.
	for(auto& [fluidType, pair] : m_block.m_fluids)
	{
		auto found = moveType.swim.find(fluidType);
		if(found != moveType.swim.end() && found->second <= pair.first)
			return true;
	}
	// Not swimming and fluid level is too high.
	if(m_block.m_totalFluidVolume > Config::maxBlockVolume / 2)
		return false;
	// Not flying and either not walking or ground is not supported.
	if(!moveType.fly && (!moveType.walk || !canStandIn()))
	{
		if(moveType.climb < 2)
			return false;
		else
		{
			// Only climb2 moveTypes can enter.
			for(Block* block : m_block.getAdjacentOnSameZLevelOnly())
				//TODO: check for climable features?
				if(block->isSupport())
					return true;
			return false;
		}
	}
	return true;
}
bool HasActors::canStandIn() const
{
	assert(m_block.m_adjacents.at(0) != nullptr);
	if(m_block.m_adjacents[0]->isSolid())
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::floor))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::upStairs))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::downStairs))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::upDownStairs))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::floorGrate))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::hatch))
		return true;
	if(m_adjacents[0]->m_block.m_hasBlockFeatures.contains(BlockFeatureType::upStairs))
		return true;
	if(m_adjacents[0]->m_block.m_hasBlockFeatures.contains(BlockFeatureType::upDownStairs))
		return true;
	// Neccessary for multi tile actors to use ramps.
	if(m_adjacents[0]->m_block.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return true;
	return false;
}
bool HasActors::canEnterCurrentlyFrom(Actor& actor, Block& block) const
{
	assert(canEnterEverFrom(actor));
	if(shape.positions.size() == 1)
	{
		uint32_t v = shape.positions[0][3];
		if(m_block == actor.m_location)
			v = 0;
		return m_block.m_totalDynamicVolume + v <= Config::maxBlockVolume;
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
		if(block->m_hasActors.shapeAndMoveTypeCanEnterEverFrom(shape, moveType, m_block))
			output.emplace_back(block, block->moveCostFrom(moveType, m_block);
	return output;
}
// Get a move cost for moving from a block onto this one for a given move type.
uint32_t HasActors::moveCostFrom(const MoveType& moveType, Block& from) const
{
	if(moveType.fly)
		return 10;
	for(auto& [fluidType, volume] : moveType.swim)
		if(m_block.volumeOfFluidTypeContains(*fluidType) >= volume)
			return 10;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(m_block.m_z > from.m_z && !from.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return 20;
	return 10;
}
void HasActors::clearCache()
{
	m_moveCostsCache.clear();
	for(Block* block : getAdjacentWithEdgeAndCornerAdjacent())
		block->m_hasActors.m_moveCostsCache.clear();
}
