#include "actors.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "../fluidType.h"
#include "../moveType.h"
#include "../types.h"
#include "../simulation.h"
#include "../util.h"
#include "../pathRequest.h"
#include "objective.h"
#include "reservable.h"
#include "terrainFacade.h"
Speed Actors::move_getIndividualSpeedWithAddedMass(ActorIndex index, Mass mass) const
{
	Speed output = m_attributes.at(index).getMoveSpeed();
	Mass carryMass = m_equipmentSet.at(index).getMass() + canPickUp_getMass(index) + mass;
	Mass unencomberedCarryMass = m_attributes.at(index).getUnencomberedCarryMass();
	if(carryMass > unencomberedCarryMass)
	{
		float ratio = (float)unencomberedCarryMass / (float)carryMass;
		if(ratio < Config::minimumOverloadRatio)
			return 0;
		output = std::ceil(output * ratio * ratio);
	}
	output = util::scaleByInversePercent(output, m_body.at(index)->getImpairMovePercent());
	return std::ceil(output);
}
void Actors::move_updateIndividualSpeed(ActorIndex index)
{
	m_speedIndividual.at(index) = move_getIndividualSpeedWithAddedMass(index, 0);
	move_updateActualSpeed(index);
}
void Actors::move_updateActualSpeed(ActorIndex index)
{
	m_speedActual.at(index) = isLeading(index) ? move_getSpeed(index) : m_speedIndividual.at(index);
}
void Actors::move_setPath(ActorIndex index, std::vector<BlockIndex>& path)
{
	assert(!path.empty());
	m_path.at(index) = std::move(path);
	m_destination.at(index) = path.back();
	m_pathIter.at(index) = path.begin();
	move_clearAllEventsAndTasks(index);
	move_schedule(index);
}
void Actors::move_clearPath(ActorIndex index)
{
	m_path.at(index).clear();
	m_pathIter.at(index) = m_path.at(index).end();
	m_destination.at(index) = BLOCK_INDEX_MAX;
	move_clearAllEventsAndTasks(index);
}
void Actors::move_callback(ActorIndex index)
{
	assert(!m_path.at(index).empty());
	assert(m_destination.at(index) != BLOCK_INDEX_MAX);
	assert(m_pathIter.at(index) >= m_path.at(index).begin());
	assert(m_pathIter.at(index) != m_path.at(index).end());
	BlockIndex block = *m_pathIter.at(index);
	// Follower is blocked, wait for them.
	// TODO: Follower cannot proceed ever, permantly blocked.
	if(m_lead.contains(index) && !m_lead.at(index).canMove(m_area))
		move_schedule(index);
	Blocks& blocks = m_area.getBlocks();
	// Path has become permanantly blocked since being generated, repath.
	if(!blocks.shape_anythingCanEnterEver(block) || !blocks.shape_shapeAndMoveTypeCanEnterEverFrom(block, *m_shape.at(index), *m_moveType.at(index), m_location.at(index)))
	{
		move_setDestination(index, m_destination.at(index));
		return;
	}
	if(blocks.shape_canEnterCurrentlyFrom(block, *m_shape.at(index), m_location.at(index), m_blocks.at(index)))
	{
		// Path is not permanantly or temporarily blocked.
		m_moveRetries.at(index) = 0;
		setLocation(index, block);
		if(block == m_destination.at(index))
		{
			m_destination.at(index) = BLOCK_INDEX_MAX;
			m_path.clear();
			m_hasObjectives.at(index).subobjectiveComplete();
		}
		else
		{
			++m_pathIter.at(index);
			assert(m_pathIter.at(index) != m_path.at(index).end());
			BlockIndex nextBlock = *m_pathIter.at(index);
			if(blocks.shape_anythingCanEnterEver(nextBlock) && blocks.shape_shapeAndMoveTypeCanEnterEverFrom(block, *m_shape.at(index), *m_moveType.at(index), m_location.at(index)))
				move_schedule(index);
			else
				m_hasObjectives.at(index).cannotCompleteSubobjective();
		}
	}
	else
	{
		// Path is temporarily blocked, wait a bit and then detour if still blocked.
		if(m_moveRetries.at(index) == Config::moveTryAttemptsBeforeDetour)
		{
			if(m_hasObjectives.at(index).hasCurrent())
				m_hasObjectives.at(index).detour();
			else
				move_setDestination(index, m_destination.at(index), true);
		}
		else
		{
			++m_moveRetries.at(index);
			move_schedule(index);
		}
	}
}
void Actors::move_schedule(ActorIndex index)
{
	assert(m_location.at(index) != m_destination.at(index));
	assert(m_pathIter.at(index) >= m_path.at(index).begin());
	assert(m_pathIter.at(index) != m_path.at(index).end());
	BlockIndex moveTo = *m_pathIter.at(index);
	m_moveEvent.schedule(index, move_delayToMoveInto(index, moveTo), *this);
}
void Actors::move_setDestination(ActorIndex index, BlockIndex destination, bool detour, bool adjacent, bool unreserved, bool reserve)
{
	if(reserve)
		assert(unreserved);
	move_clearPath(index);
	Blocks& blocks = m_area.getBlocks();
	// If adjacent path then destination isn't known until it's completed.
	if(!adjacent)
	{
		m_destination.at(index) = destination;
		assert(blocks.shape_anythingCanEnterEver(destination));
		assert(m_location.at(index) != m_destination.at(index));
	}
	else
		assert(!isAdjacentToLocation(index, destination));
	move_clearAllEventsAndTasks(index);
	if(m_faction.at(index) == nullptr)
		reserve = unreserved = false;
	if(unreserved && !adjacent)
		assert(!blocks.isReserved(destination, *m_faction.at(index)));
	if(adjacent)
		m_pathRequest[index]->createGoAdjacentToLocation(m_area, index, destination, detour, unreserved, BLOCK_DISTANCE_MAX, reserve);
	else
		m_pathRequest[index]->createGoTo(m_area, index, destination, detour, unreserved, BLOCK_DISTANCE_MAX, reserve);
}
void Actors::move_setDestinationAdjacentToLocation(ActorIndex index, BlockIndex destination, bool detour, bool unreserved, bool reserve)
{
	move_setDestination(index, destination, detour, true, unreserved, reserve);
}
void Actors::move_setDestinationAdjacentToActor(ActorIndex index, ActorIndex other, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToActor(index, other));
	assert(!isAdjacentToLocation(index, m_location.at(other)));
	// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
	m_pathRequest[index]->createGoAdjacentToActor(m_area, index, other, detour, unreserved, BLOCK_DISTANCE_MAX, reserve);
}
void Actors::move_setDestinationAdjacentToItem(ActorIndex index, ItemIndex item, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToItem(index, item));
	assert(!isAdjacentToLocation(index, m_location.at(item)));
	m_pathRequest[index]->createGoAdjacentToItem(m_area, index, item, detour, unreserved, BLOCK_DISTANCE_MAX, reserve);
}
void Actors::move_setDestinationAdjacentToPlant(ActorIndex index, PlantIndex item, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToPlant(index, item));
	assert(!isAdjacentToLocation(index, m_location.at(item)));
	m_pathRequest[index]->createGoAdjacentToPlant(m_area, index, plant, detour, unreserved, BLOCK_DISTANCE_MAX, reserve);
}
void Actors::move_setDestinationAdjacentToFluidType(ActorIndex index, const FluidType& fluidType, bool detour, bool unreserved, bool reserve, DistanceInBlocks maxRange)
{
	m_pathRequest[index]->createGoAdjacentToFluidType(m_area, index, fluidType, detour, unreserved, maxRange, reserve);
}
void Actors::move_setDestinationAdjacentToDesignation(ActorIndex index, BlockDesignation designation, bool detour, bool unreserved, bool reserve, DistanceInBlocks maxRange)
{
	m_pathRequest[index]->createGoAdjacentToDesignation(m_area, index, designation, detour, unreserved, maxRange, reserve);
}
void Actors::move_setType(ActorIndex index, const MoveType& moveType)
{
	assert(m_moveType.at(index) != &moveType);
	m_moveType.at(index) = &moveType;
	// TODO: unregister move type: decrement count of actors using a given facade and maybe clear when 0.
	m_area.m_hasTerrainFacades.maybeRegisterMoveType(moveType);
	//TODO: repath if destination, possibly do something with pathRequest?
}
void Actors::move_clearAllEventsAndTasks(ActorIndex index)
{
	m_moveEvent.maybeUnschedule(index);
	move_pathRequestMaybeCancel(index);
}
void Actors::move_onLeaveArea(ActorIndex index) { move_clearAllEventsAndTasks(index); }
void Actors::move_onDeath(ActorIndex index) { move_clearAllEventsAndTasks(index); }
bool Actors::move_tryToReserveProposedDestination(ActorIndex index, std::vector<BlockIndex>& path)
{
	const Shape& shape = getShape(index);
	CanReserve& canReserve = m_canReserve.at(index);
	Blocks& blocks = m_area.getBlocks();
	BlockIndex location = path.back();
	Faction& faction = *getFaction(index);
	if(shape.isMultiTile)
	{
		assert(!path.empty());
		BlockIndex previous = path.size() == 1 ? getLocation(index) : path.at(path.size() - 2);
		Facing facing = blocks.facingToSetWhenEnteringFrom(location, previous);
		auto occupiedBlocks = shape.getBlocksOccupiedAt(blocks, location, facing);
		for(BlockIndex block : occupiedBlocks)
			if(blocks.isReserved(block, faction))
				return false;
		for(BlockIndex block : occupiedBlocks)
			blocks.reserve(block, canReserve);
	}
	else
	{
		if(blocks.isReserved(location, faction))
			return false;
		blocks.reserve(location, canReserve);
	}
}
bool Actors::move_tryToReserveOccupied(ActorIndex index)
{
	const Shape& shape = getShape(index);
	CanReserve& canReserve = m_canReserve.at(index);
	Blocks& blocks = m_area.getBlocks();
	BlockIndex location = getLocation(index);
	Faction& faction = *getFaction(index);
	if(shape.isMultiTile)
	{
		assert(!m_path.at(index).empty());
		auto occupiedBlocks = getBlocks(index);
		for(BlockIndex block : occupiedBlocks)
			if(blocks.isReserved(block, faction))
				return false;
		for(BlockIndex block : occupiedBlocks)
			blocks.reserve(block, canReserve);
	}
	else
	{
		if(blocks.isReserved(location, faction))
			return false;
		blocks.reserve(location, canReserve);
	}
}
void Actors::move_pathRequestCallback(ActorIndex index, std::vector<BlockIndex> path, bool useCurrentLocation, bool reserveDestination)
{
	if(path.empty() || (reserveDestination && !move_tryToReserveDestination(index)))
	{
		if(useCurrentLocation && (!reserveDestination || move_tryToReserveOccupied(index)))
		{
			// The actor's current location is already acceptable.
			m_hasObjectives.at(index).subobjectiveComplete();
			return;
		}
		// TODO:
		// Should this be restart, or even cannotCompleteSubobjective?
		// cannotFulfill will cause a delay to be appiled before this objective can be added to the list again.
		// restart or cannotCompleteSubobjective might cause an infinite loop.
		// To prevent this we would have to know if we may reroute or not.
		// cannotFulfillObjective is the most pessamistic choice.
		// This is probably best solved by callback.
		//TODO: This conditional only exists because objectives are not mandated to always exist for all actors.
		HasObjectives& hasObjectives = m_hasObjectives.at(index);
		if(hasObjectives.hasCurrent())
		{
			// Objectives can handle the path not found situation directly or default to cannotFulfill.
			Objective& objective = hasObjectives.getCurrent();
			if(!objective.onCanNotRepath(m_area))
			{
				if(objective.isNeed())
					hasObjectives.cannotFulfillNeed(objective);
				else
					hasObjectives.cannotFulfillObjective(objective);
			}
		}
	}
	else
		move_setPath(index, path);
}
void Actors::move_pathRequestMaybeCancel(ActorIndex index)
{
	if(m_pathRequest.at(index) != nullptr)
		m_pathRequest.at(index)->cancel(m_area, index);
}
bool Actors::move_canMove(ActorIndex index) const
{
	if(!isAlive(index) || m_speedIndividual.at(index) == 0)
		return false;
	return true;
}
Step Actors::move_delayToMoveInto(ActorIndex index, BlockIndex block) const
{
	Speed speed = m_speedActual.at(index);
	CollisionVolume volumeAtLocationBlock = m_shape.at(index)->getCollisionVolumeAtLocationBlock();
	assert(block != m_location.at(index));
	Blocks& blocks = m_area.getBlocks();
	if(volumeAtLocationBlock + blocks.shape_getDynamicVolume(block) > Config::maxBlockVolume)
	{
		CollisionVolume excessVolume = (volumeAtLocationBlock + blocks.shape_getStaticVolume(block)) - Config::maxBlockVolume;
		speed = util::scaleByInversePercent(speed, excessVolume);
	}
	assert(speed);
	static const MoveCost stepsPerSecond = Config::stepsPerSecond;
	MoveCost cost = blocks.shape_moveCostFrom(block, *m_moveType.at(index), m_location.at(index));
	assert(cost);
	return std::max(1u, uint(std::round(float(stepsPerSecond * cost) / float(speed))));
}
Step Actors::move_stepsTillNextMoveEvent(ActorIndex index) const
{
	// Add 1 because we increment step number at the end of the step.
       	return 1 + m_moveEvent.getStep(index) - m_area.m_simulation.m_step;
}
// MoveEvent
MoveEvent::MoveEvent(Step delay, Area& area, ActorIndex actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(actor) { }
void MoveEvent::execute(Simulation&, Area* area) { area->m_actors.move_callback(m_actor); }
void MoveEvent::clearReferences(Simulation &, Area *area) { area->m_actors.m_moveEvent.clearPointer(m_actor); }
