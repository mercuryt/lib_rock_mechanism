#include "blocks.h"
#include "../actors/actors.h"
#include "../area.h"
#include "types.h"
void Blocks::actor_record(BlockIndex index, ActorIndex actor, CollisionVolume volume)
{
	Actors& actors = m_area.getActors();
	m_actorVolume.at(index).emplace_back(actor, volume);
	m_actors.at(index).add(actor);
	if(actors.isStatic(actor))
		m_staticVolume.at(index) += volume;
	else
		m_dynamicVolume.at(index) += volume;
}
void Blocks::actor_erase(BlockIndex index, ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	auto& blockActors = m_actors.at(index);
	auto& blockActorVolume = m_actorVolume.at(index);
	auto iter = std::ranges::find(blockActors, actor);
	auto iter2 = m_actorVolume.at(index).begin() + (std::distance(iter, blockActors.begin()));
	if(actors.isStatic(actor))
		m_staticVolume.at(index) -= iter2->second;
	else
		m_dynamicVolume.at(index) -= iter2->second;
	blockActors.remove(iter);
	(*iter2) = blockActorVolume.back();
	blockActorVolume.pop_back();
}
void Blocks::actor_setTemperature(BlockIndex index, Temperature temperature)
{
	assert(temperature_get(index) == temperature);
	Actors& actors = m_area.getActors();
	for(auto pair : m_actorVolume.at(index))
		actors.temperature_onChange(pair.first);
}
void Blocks::actor_updateIndex(BlockIndex index, ActorIndex oldIndex, ActorIndex newIndex)
{
	auto found = std::ranges::find(m_actors.at(index), oldIndex);
	assert(found != m_actors.at(index).end());
	(*found) = newIndex; 
	auto found2 = std::ranges::find(m_actorVolume.at(index), oldIndex, &std::pair<ActorIndex, CollisionVolume>::first);
	assert(found2 != m_actorVolume.at(index).end());
	found2->first = newIndex; 
}
bool Blocks::actor_contains(BlockIndex index, ActorIndex actor) const
{
	return m_actors.at(index).contains(actor);
}
bool Blocks::actor_canEnterCurrentlyFrom(BlockIndex index, ActorIndex actor, BlockIndex block) const
{
	Facing facing = facingToSetWhenEnteringFrom(index, block);
	return actor_canEnterCurrentlyWithFacing(index, actor, facing);
}
bool Blocks::actor_canEnterCurrentlyWithFacing(BlockIndex index, ActorIndex actor, Facing facing) const
{
	Actors& actors = m_area.getActors();
	const Shape& shape = actors.getShape(actor);
	// For multi block shapes assume that volume is the same for each block.
	CollisionVolume volume = shape.getCollisionVolumeAtLocationBlock();
	for(BlockIndex block : shape.getBlocksOccupiedAt(*this, index, facing))
		if(!actor_contains(block, actor) && (/*m_actors.at(block).full() ||*/ m_dynamicVolume.at(block) + volume > Config::maxBlockVolume))
				return false;
	return true;
}
bool Blocks::actor_canEnterEverOrCurrentlyWithFacing(BlockIndex index, ActorIndex actor, const Facing facing) const
{
	assert(shape_anythingCanEnterEver(index));
	const Actors& actors = m_area.getActors();
	const MoveType& moveType = actors.getMoveType(actor);
	for(auto& [x, y, z, v] : actors.getShape(actor).positionsWithFacing(facing))
	{
		BlockIndex block = offset(index,x, y, z);
		if(m_actors.at(block).contains(actor))
			continue;
		if(block.empty() || !shape_anythingCanEnterEver(block) ||
			/*m_actors.at(block).full() ||*/
			m_dynamicVolume.at(block) + v > Config::maxBlockVolume || 
			!shape_moveTypeCanEnter(block, moveType)
		)
			return false;
	}
	return true;
}
bool Blocks::actor_canEnterCurrentlyWithAnyFacing(BlockIndex index, ActorIndex actor) const
{
	for(Facing i = Facing::create(0); i < 4; ++i)
		if(actor_canEnterCurrentlyWithFacing(index, actor, i))
			return true;
	return false;
}
bool Blocks::actor_empty(BlockIndex index) const
{
	return m_actors.at(index).empty();
}
ActorIndicesForBlock& Blocks::actor_getAll(BlockIndex index)
{
	return m_actors.at(index);
}
