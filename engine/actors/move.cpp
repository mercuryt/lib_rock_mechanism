#include "actors.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "../fluidType.h"
#include "../moveType.h"
#include "../types.h"
#include "../simulation.h"
#include "../util.h"
#include "../pathRequest.h"
#include "../blocks/blocks.h"
#include "config.h"
#include "eventSchedule.h"
Speed Actors::move_getIndividualSpeedWithAddedMass(ActorIndex index, Mass mass) const
{
	Speed output = Speed::create(m_agility[index].get() * Config::unitsOfMoveSpeedPerUnitOfAgility);
	Mass carryMass = m_equipmentSet[index]->getMass() + canPickUp_getMass(index) + mass;
	Mass unencomberedCarryMass = m_unencomberedCarryMass[index];
	if(carryMass > unencomberedCarryMass)
	{
		float ratio = (float)unencomberedCarryMass.get() / (float)carryMass.get();
		if(ratio < Config::minimumOverloadRatio)
			return Speed::create(0);
		output = Speed::create(std::ceil(output.get() * ratio * ratio));
	}
	output = Speed::create(util::scaleByInversePercent(output.get(), m_body[index]->getImpairMovePercent()));
	return Speed::create(std::ceil(output.get()));
}
void Actors::move_updateIndividualSpeed(ActorIndex index)
{
	m_speedIndividual[index] = move_getIndividualSpeedWithAddedMass(index, Mass::create(0));
	move_updateActualSpeed(index);
}
void Actors::move_updateActualSpeed(ActorIndex index)
{
	m_speedActual[index] = isLeading(index) ? lead_getSpeed(index) : m_speedIndividual[index];
}
void Actors::move_setPath(ActorIndex index, BlockIndices& path)
{
	assert(!path.empty());
	assert(m_area.getBlocks().isAdjacentToIncludingCornersAndEdges(m_location[index], path[0]));
	m_path[index] = std::move(path);
	m_destination[index] = m_path[index].back();
	m_pathIter[index] = m_path[index].begin();
	move_clearAllEventsAndTasks(index);
	move_schedule(index);
}
void Actors::move_clearPath(ActorIndex index)
{
	m_path[index].clear();
	m_pathIter[index] = m_path[index].end();
	m_destination[index].clear();
	move_clearAllEventsAndTasks(index);
}
void Actors::move_callback(ActorIndex index)
{
	assert(!m_path[index].empty());
	assert(m_destination[index].exists());
	assert(m_pathIter[index] >= m_path[index].begin());
	assert(m_pathIter[index] != m_path[index].end());
	BlockIndex block = *m_pathIter[index];
	Blocks& blocks = m_area.getBlocks();
	// Path has become permanantly blocked since being generated, repath.
	if(!blocks.shape_anythingCanEnterEver(block) || !blocks.shape_shapeAndMoveTypeCanEnterEverFrom(block, m_shape[index], m_moveType[index], m_location[index]))
	{
		move_setDestination(index, m_destination[index]);
		return;
	}
	if(!blocks.shape_canEnterCurrentlyFrom(block, m_shape[index], m_location[index], m_blocks[index]))
	{
		// Path is temporarily blocked, wait a bit and then detour if still blocked.
		if(m_moveRetries[index] == Config::moveTryAttemptsBeforeDetour)
		{
			if(m_hasObjectives[index]->hasCurrent())
				m_hasObjectives[index]->detour(m_area);
			else
				move_setDestination(index, m_destination[index], true);
		}
		else
		{
			++m_moveRetries[index];
			move_schedule(index);
		}
	}
	else
	{
		// Path is not blocked for actor. Check for followers, if any.
		ActorOrItemIndex follower = getFollower(index);
		if(follower.exists())
		{
			const auto &path = lineLead_getPath(index);
			while (follower.exists())
			{
				auto found = std::ranges::find(path, follower.getLocation(m_area));
				assert(found != path.begin());
				BlockIndex followerNextStep = *(--found);
				if (!blocks.shape_anythingCanEnterEver(followerNextStep) || !blocks.shape_shapeAndMoveTypeCanEnterEverFrom(block, follower.getShape(m_area), follower.getMoveType(m_area), follower.getLocation(m_area)))
				{
					// Path is permanantly blocked for follower, repath.
					move_setDestination(index, m_destination[index]);
					return;
				}
				if (!follower.canEnterCurrentlyFrom(m_area, followerNextStep, follower.getLocation(m_area)))
				{
					// TODO: this repeats 12 lines from above starting at 68.
					// Path is blocked temporarily, wait.
					if (m_moveRetries[index] == Config::moveTryAttemptsBeforeDetour)
					{
						if (m_hasObjectives[index]->hasCurrent())
							m_hasObjectives[index]->detour(m_area);
						else
							move_setDestination(index, m_destination[index], true);
					}
					else
					{
						++m_moveRetries[index];
						move_schedule(index);
						return;
					}
				}
				follower = follower.getFollower(m_area);
			}
		}
		// Path is not blocked for followers.
		m_moveRetries[index] = 0;
		Facing facing = blocks.facingToSetWhenEnteringFrom(block, m_location[index]);
		setLocationAndFacing(index, block, facing);
		// Move followers.
		follower = getFollower(index);
		if(follower.exists())
		{
			auto& path = m_leadFollowPath[index];
			while (follower.exists())
			{
				BlockIndex currentFollowerLocation = follower.getLocation(m_area);
				auto found = std::ranges::find(path, currentFollowerLocation);
				assert(found != path.begin());
				assert(found != path.end());
				BlockIndex followerNextStep = *(--found);
				Facing facing = blocks.facingToSetWhenEnteringFrom(followerNextStep, currentFollowerLocation);
				follower.setLocationAndFacing(m_area, followerNextStep, facing);
				follower = follower.getFollower(m_area);
			}
		}
		if(block == m_destination[index])
		{
			m_destination[index].clear();
			m_path[index].clear();
			m_hasObjectives[index]->subobjectiveComplete(m_area);
		}
		else
		{
			++m_pathIter[index];
			assert(m_pathIter[index] != m_path[index].end());
			BlockIndex nextBlock = *m_pathIter[index];
			if(blocks.shape_anythingCanEnterEver(nextBlock) && blocks.shape_shapeAndMoveTypeCanEnterEverFrom(block, m_shape[index], m_moveType[index], m_location[index]))
				move_schedule(index);
			else
				m_hasObjectives[index]->cannotCompleteSubobjective(m_area);
		}
	}
}
void Actors::move_schedule(ActorIndex index)
{
	assert(m_location[index] != m_destination[index]);
	assert(m_pathIter[index] >= m_path[index].begin());
	assert(m_pathIter[index] != m_path[index].end());
	BlockIndex moveTo = *m_pathIter[index];
	m_moveEvent.schedule(index, move_delayToMoveInto(index, moveTo), m_area, index);
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
		m_destination[index] = destination;
		assert(blocks.shape_anythingCanEnterEver(destination));
		assert(m_location[index] != m_destination[index]);
	}
	else
		assert(getLocation(index) != destination);
	move_clearAllEventsAndTasks(index);
	if(m_faction[index].empty())
		reserve = unreserved = false;
	if(unreserved && !adjacent)
		assert(!blocks.isReserved(destination, m_faction[index]));
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	if(adjacent)
		m_pathRequest[index]->createGoAdjacentToLocation(m_area, index, destination, detour, unreserved, DistanceInBlocks::null(), reserve);
	else
		m_pathRequest[index]->createGoTo(m_area, index, destination, detour, unreserved, DistanceInBlocks::null(), reserve);
}
void Actors::move_setDestinationAdjacentToLocation(ActorIndex index, BlockIndex destination, bool detour, bool unreserved, bool reserve)
{
	move_setDestination(index, destination, detour, true, unreserved, reserve);
}
void Actors::move_setDestinationAdjacentToActor(ActorIndex index, ActorIndex other, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToActor(index, other));
	assert(!isAdjacentToLocation(index, m_location[other]));
	// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToActor(m_area, index, other, detour, unreserved, DistanceInBlocks::null(), reserve);
}
void Actors::move_setDestinationAdjacentToItem(ActorIndex index, ItemIndex item, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToItem(index, item));
	assert(!isAdjacentToLocation(index, m_location[item]));
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToItem(m_area, index, item, detour, unreserved, DistanceInBlocks::null(), reserve);
}
void Actors::move_setDestinationAdjacentToPlant(ActorIndex index, PlantIndex plant, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToPlant(index, plant));
	assert(!isAdjacentToLocation(index, m_location[plant]));
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToPlant(m_area, index, plant, detour, unreserved, DistanceInBlocks::null(), reserve);
}
void Actors::move_setDestinationAdjacentToPolymorphic(ActorIndex index, ActorOrItemIndex actorOrItemIndex, bool detour, bool unreserved, bool reserve)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
		move_setDestinationAdjacentToActor(index, actorOrItemIndex.getActor(), detour, unreserved, reserve);
	else
		move_setDestinationAdjacentToItem(index, actorOrItemIndex.getItem(), detour, unreserved, reserve);
}
void Actors::move_setDestinationAdjacentToFluidType(ActorIndex index, FluidTypeId fluidType, bool detour, bool unreserved, bool reserve, DistanceInBlocks maxRange)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToFluidType(m_area, index, fluidType, detour, unreserved, maxRange, reserve);
}
void Actors::move_setDestinationAdjacentToDesignation(ActorIndex index, BlockDesignation designation, bool detour, bool unreserved, bool reserve, DistanceInBlocks maxRange)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToDesignation(m_area, index, designation, detour, unreserved, maxRange, reserve);
}
void Actors::move_setDestinationToEdge(ActorIndex index, bool detour)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoToEdge(m_area, index, detour);
}
void Actors::move_setType(ActorIndex index, MoveTypeId moveType)
{
	assert(m_moveType[index] != moveType);
	m_moveType[index] = moveType;
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
bool Actors::move_tryToReserveProposedDestination(ActorIndex index, BlockIndices& path)
{
	ShapeId shape = getShape(index);
	CanReserve& canReserve = canReserve_get(index);
	Blocks& blocks = m_area.getBlocks();
	BlockIndex location = path.back();
	FactionId faction = getFactionId(index);
	if(Shape::getIsMultiTile(shape))
	{
		assert(!path.empty());
		BlockIndex previous = path.size() == 1 ? getLocation(index) : path[path.size() - 2];
		Facing facing = blocks.facingToSetWhenEnteringFrom(location, previous);
		auto occupiedBlocks = Shape::getBlocksOccupiedAt(shape, blocks, location, facing);
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
	return true;
}
bool Actors::move_tryToReserveOccupied(ActorIndex index)
{
	ShapeId shape = getShape(index);
	CanReserve& canReserve = canReserve_get(index);
	Blocks& blocks = m_area.getBlocks();
	BlockIndex location = getLocation(index);
	FactionId faction = getFactionId(index);
	if(Shape::getIsMultiTile(shape))
	{
		assert(!m_path[index].empty());
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
	return true;
}
void Actors::move_pathRequestCallback(ActorIndex index, BlockIndices path, bool useCurrentLocation, bool reserveDestination)
{
	if(path.empty() || (reserveDestination && !move_tryToReserveProposedDestination(index, path)))
	{
		if(useCurrentLocation && (!reserveDestination || move_tryToReserveOccupied(index)))
		{
			// The actor's current location is already acceptable.
			m_hasObjectives[index]->subobjectiveComplete(m_area);
			return;
		}
		// TODO:
		// Should this be restart, or even cannotCompleteSubobjective?
		// cannotFulfill will cause a delay to be appiled before this objective can be added to the list again.
		// restart or cannotCompleteSubobjective might cause an infinite loop.
		// To prevent this we would have to know if we may reroute or not.
		// cannotFulfillObjective is the most pessamistic choice.
		// This is probably best solved by callback.
		HasObjectives& hasObjectives = *m_hasObjectives[index];
		//TODO: This conditional only exists because objectives are not mandated to always exist for all actors.
		if(hasObjectives.hasCurrent())
		{
			// Objectives can handle the path not found situation directly or default to cannotFulfill.
			Objective& objective = hasObjectives.getCurrent();
			if(!objective.onCanNotRepath(m_area, index))
			{
				if(objective.isNeed())
					hasObjectives.cannotFulfillNeed(m_area, objective);
				else
					hasObjectives.cannotFulfillObjective(m_area, objective);
			}
		}
	}
	else
		move_setPath(index, path);
}
void Actors::move_pathRequestRecord(ActorIndex index, std::unique_ptr<PathRequest> pathRequest)
{
	assert(m_pathRequest[index] == nullptr);
	assert(pathRequest->exists());
	m_pathRequest[index] = std::move(pathRequest);
}
void Actors::move_pathRequestMaybeCancel(ActorIndex index)
{
	if(m_pathRequest[index] != nullptr)
	{
		m_pathRequest[index]->cancel(m_area, index);
		m_pathRequest[index] = nullptr;
	}
}
bool Actors::move_canMove(ActorIndex index) const
{
	if(!isAlive(index) || m_speedIndividual[index] == 0)
		return false;
	return true;
}
Step Actors::move_delayToMoveInto(ActorIndex index, BlockIndex block) const
{
	Speed speed = m_speedActual[index];
	CollisionVolume volumeAtLocationBlock = Shape::getCollisionVolumeAtLocationBlock(m_shape[index]);
	assert(block != m_location[index]);
	Blocks& blocks = m_area.getBlocks();
	if(volumeAtLocationBlock + blocks.shape_getDynamicVolume(block) > Config::maxBlockVolume)
	{
		CollisionVolume excessVolume = (volumeAtLocationBlock + blocks.shape_getStaticVolume(block)) - Config::maxBlockVolume;
		speed = Speed::create(util::scaleByPercent(speed.get(), Percent::create(100 - excessVolume.get())));
	}
	assert(speed != 0);
	static const Step stepsPerSecond = Config::stepsPerSecond;
	MoveCost cost = blocks.shape_moveCostFrom(block, m_moveType[index], m_location[index]);
	assert(cost != 0);
	return Step::create(std::max(1u, uint(std::round(float(stepsPerSecond.get() * cost.get()) / float(speed.get())))));
}
Step Actors::move_stepsTillNextMoveEvent(ActorIndex index) const
{
	// Add 1 because we increment step number at the end of the step.
       	return Step::create(1) + m_moveEvent.getStep(index) - m_area.m_simulation.m_step;
}
// MoveEvent
MoveEvent::MoveEvent(Step delay, Area& area, ActorIndex actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(actor) { }
MoveEvent::MoveEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data)
{
	data["actor"].get_to(m_actor);
}
void MoveEvent::execute(Simulation&, Area* area) { area->getActors().move_callback(m_actor); }
void MoveEvent::clearReferences(Simulation &, Area *area) { area->getActors().m_moveEvent.clearPointer(m_actor); }
Json MoveEvent::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["actor"] = m_actor;
	return output;
}
