#include "actor.h"
#include "block.h"
#include "area.h"

#include <algorithm>

Actor::Actor(uint32_t id, const std::wstring name, const AnimalSpecies& species, uint32_t percentGrown, Faction& faction, Attributes attributes) :
	HasShape(species.shapeForPercentGrown(percentGrown)), m_id(id), m_name(name), m_species(species), m_alive(true), m_awake(true), m_faction(&faction), m_attributes(attributes), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_canMove(*this), m_canFight(*this), m_canPickup(*this), m_canGrow(*this, percentGrown), m_hasObjectives(*this) { }
void Actor::setLocation(Block& block)
{
	assert(&block != HasShape::m_location);
	assert(block.m_hasActors.anyoneCanEnterEver());
	if(m_location == nullptr)
	{
		assert(block.m_hasActors.canEnterEverWithFacing(*this, m_facing));
		assert(block.m_hasActors.canEnterCurrentlyWithFacing(*this, m_facing));
	}
	else
	{
		assert(block.m_hasActors.canEnterEverFrom(*this, *m_location));
		assert(block.m_hasActors.canEnterCurrentlyFrom(*this, *m_location));
	}
	block.m_hasActors.enter(*this);
}
bool Actor::isEnemy(Actor& actor) const { return m_faction->m_enemies.contains(actor.m_faction); }
bool Actor::isAlly(Actor& actor) const { return m_faction->m_allies.contains(actor.m_faction); }
void Actor::die()
{
	m_canFight.onDie();
	m_location->m_area->unregisterActor(*this);
}
void Actor::passout(uint32_t duration)
{
	//TODO
	(void)duration;
}
// ActorQuery, to be used to search for actors.
bool ActorQuery::operator()(Actor& other) const
{
	if(actor != nullptr)
		return actor == &other;
	if(carryWeight != 0 && other.m_canPickup.getCarryWeight() < carryWeight)
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
	assert(actor.m_location != &m_block);
	assert(!contains(actor));
	if(actor.m_location != nullptr)
	{
		actor.m_location->m_hasActors.exit(actor);
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_location);
		m_block.m_area->m_actorLocationBuckets.update(actor, *actor.m_location, m_block);
	}
	else
		m_block.m_area->m_actorLocationBuckets.insert(actor);
	actor.m_location = &m_block;
	for(auto& [m_x, m_y, m_z, v] : actor.m_shape->positionsWithFacing(actor.m_facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		assert(block != nullptr);
		assert(!block->isSolid());
		assert(!block->m_hasActors.contains(actor));
		block->m_hasActors.m_volume += v;
		block->m_hasActors.m_actors.emplace_back(&actor, v);
		actor.m_blocks.insert(block);
	}
}
void HasActors::exit(Actor& actor)
{
	assert(actor.m_location == &m_block);
	assert(contains(actor));
	for(auto& [m_x, m_y, m_z, v] : actor.m_shape->positionsWithFacing(actor.m_facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		assert(block != nullptr);
		assert(!block->isSolid());
		assert(block->m_hasActors.contains(actor));
		block->m_hasActors.m_volume -= v;
		std::erase_if(block->m_hasActors.m_actors, [&](auto pair){ return pair.first == &actor; });
	}
	actor.m_location = nullptr;
	actor.m_blocks.clear();
}
void HasActors::clearCache()
{
	m_moveCostsCache.clear();
	for(Block* block : m_block.getAdjacentWithEdgeAndCornerAdjacent())
		block->m_hasActors.m_moveCostsCache.clear();
}
bool HasActors::anyoneCanEnterEver() const
{
	if(m_block.isSolid())
		return false;
	// TODO: cache this.
	return !m_block.m_hasBlockFeatures.blocksEntrance();
}
bool HasActors::canEnterEverFrom(Actor& actor, Block& block) const
{
	return shapeAndMoveTypeCanEnterEverFrom(*actor.m_shape, actor.m_canMove.getMoveType(), block);
}
bool HasActors::canEnterEverWithFacing(Actor& actor, const uint8_t facing) const
{
	return shapeAndMoveTypeCanEnterEverWithFacing(*actor.m_shape, actor.m_canMove.getMoveType(), facing);
}
bool HasActors::shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, Block& from) const
{
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(from);
	return shapeAndMoveTypeCanEnterEverWithFacing(shape, moveType, facing);
}
bool HasActors::shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const uint8_t facing) const
{
	for(auto& [m_x, m_y, m_z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(m_x, m_y, m_z);
		if(block == nullptr)
			return false;
		if(block->isSolid())
			return false;
		if(!block->m_hasActors.moveTypeCanEnter(moveType))
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
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::stairs))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::floorGrate))
		return true;
	if(m_block.m_hasBlockFeatures.contains(BlockFeatureType::hatch))
		return true;
	if(m_block.m_adjacents[0]->m_hasBlockFeatures.contains(BlockFeatureType::stairs))
		return true;
	// Neccessary for multi tile actors to use ramps.
	if(m_block.m_adjacents[0]->m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return true;
	return false;
}
bool HasActors::canEnterCurrentlyFrom(Actor& actor, Block& block) const
{
	assert(canEnterEverFrom(actor, block));
	if(actor.m_shape->positions.size() == 1)
	{
		uint32_t v = actor.m_shape->positions[0][3];
		if(&m_block == actor.m_location)
			v = 0;
		return m_block.m_hasActors.m_volume + v <= Config::maxBlockVolume;
	}
	const uint8_t facing = m_block.facingToSetWhenEnteringFrom(block);
	return canEnterCurrentlyWithFacing(actor, facing);
}
bool HasActors::canEnterCurrentlyWithFacing(Actor& actor, uint8_t facing) const
{
	for(auto& [m_x, m_y, m_z, v] : actor.m_shape->positionsWithFacing(facing))
	{
		const Block* block = m_block.offset(m_x, m_y, m_z);
		uint32_t volumeOfActor = block->m_hasActors.volumeOf(actor);
		if(m_volume + v - volumeOfActor > Config::maxBlockVolume)
			return false;
	}
	return true;
}
const std::vector<std::pair<Block*, uint32_t>>& HasActors::getMoveCosts(const Shape& shape, const MoveType& moveType)
{
	if(m_moveCostsCache.contains(&shape) && m_moveCostsCache.at(&shape).contains(&moveType))
		return m_moveCostsCache.at(&shape).at(&moveType);
	auto& output = m_moveCostsCache[&shape][&moveType];
	for(Block* block : m_block.m_adjacentsVector)
		if(block->m_hasActors.shapeAndMoveTypeCanEnterEverFrom(shape, moveType, m_block))
			output.emplace_back(block, block->m_hasActors.moveCostFrom(moveType, m_block));
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
bool HasActors::contains(Actor& actor) const
{
	for(auto& pair : m_actors)
		if(pair.first == &actor)
			return true;
	return false;
}
uint32_t HasActors::volumeOf(Actor& actor) const
{
	for(auto& pair : m_actors)
		if(pair.first == &actor)
			return pair.second;
	return 0;
}
