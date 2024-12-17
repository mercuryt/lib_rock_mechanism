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
#include "../portables.hpp"
#include "config.h"
#include "eventSchedule.h"
#include "items/items.h"
#include "terrainFacade.h"
#include <ranges>
Speed Actors::move_getIndividualSpeedWithAddedMass(const ActorIndex& index, const Mass& mass) const
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
void Actors::move_updateIndividualSpeed(const ActorIndex& index)
{
	m_speedIndividual[index] = move_getIndividualSpeedWithAddedMass(index, Mass::create(0));
	move_updateActualSpeed(index);
}
void Actors::move_updateActualSpeed(const ActorIndex& index)
{
	m_speedActual[index] = isLeading(index) ? lead_getSpeed(index) : m_speedIndividual[index];
}
void Actors::move_setPath(const ActorIndex& index, const BlockIndices& path)
{
	assert(!path.empty());
	assert(m_area.getBlocks().isAdjacentToIncludingCornersAndEdges(m_location[index], path[0]));
	m_path[index] = std::move(path);
	m_destination[index] = m_path[index].back();
	m_pathIter[index] = m_path[index].begin();
	move_clearAllEventsAndTasks(index);
	move_schedule(index);
}
void Actors::move_clearPath(const ActorIndex& index)
{
	m_path[index].clear();
	m_pathIter[index] = m_path[index].end();
	move_clearAllEventsAndTasks(index);
}
void Actors::move_callback(const ActorIndex& index)
{
	assert(!m_path[index].empty());
	assert(m_destination[index].exists());
	assert(m_pathIter[index] >= m_path[index].begin());
	assert(m_pathIter[index] != m_path[index].end());
	BlockIndex block = *m_pathIter[index];
	Blocks& blocks = m_area.getBlocks();
	BlockIndices occupiedBlocks = isLeading(index) ? lineLead_getOccupiedBlocks(index) : m_blocks[index];
	// Path has become permanantly blocked since being generated, repath.
	if(!blocks.shape_anythingCanEnterEver(block) || !blocks.shape_shapeAndMoveTypeCanEnterEverFrom(block, m_shape[index], m_moveType[index], m_location[index]))
	{
		move_setDestination(index, m_destination[index]);
		return;
	}
	if(!blocks.shape_canEnterCurrentlyFrom(block, m_shape[index], m_location[index], occupiedBlocks))
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
			const auto& path = lineLead_getPath(index);
			while (follower.exists())
			{
				BlockIndex currentFollowerLocation = follower.getLocation(m_area);
				auto found = std::ranges::find(std::views::reverse(path), currentFollowerLocation);
				assert(found != std::ranges::end(std::views::reverse(path)));
				if(path.size() != 1)
					assert(found != std::ranges::end(std::views::reverse(path)) - 1);
				BlockIndex followerNextStep = *(found + 1);
				if (!blocks.shape_anythingCanEnterEver(followerNextStep) || !blocks.shape_shapeAndMoveTypeCanEnterEverFrom(followerNextStep, follower.getShape(m_area), follower.getMoveType(m_area), follower.getLocation(m_area)))
				{
					// Path is permanantly blocked for follower, repath.
					move_setDestination(index, m_destination[index]);
					return;
				}
				if (!follower.canEnterCurrentlyFromWithOccupied(m_area, followerNextStep, follower.getLocation(m_area), occupiedBlocks))
				{
					// TODO: this repeats 12 lines from above starting at 68.
					// Path is blocked temporarily, wait until retries are exhausted, then repath.
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
			lineLead_pushFront(index, block);
			auto& path = m_leadFollowPath[index];
			while (follower.exists())
			{
				BlockIndex currentFollowerLocation = follower.getLocation(m_area);
				auto found = std::ranges::find(std::views::reverse(path), currentFollowerLocation);
				assert(found != std::ranges::end(std::views::reverse(path)));
				if(path.size() != 1)
					assert(found != std::ranges::end(std::views::reverse(path)) - 1);
				BlockIndex followerNextStep = *(found + 1);
				Facing facing = blocks.facingToSetWhenEnteringFrom(followerNextStep, currentFollowerLocation);
				// TODO: check if follower can enter currently, it may be block by it's leader moving but not vacating, perhaps due to turning.
				follower.setLocationAndFacing(m_area, followerNextStep, facing);
				follower = follower.getFollower(m_area);
			}
			lineLead_popBackUnlessOccupiedByFollower(index);
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
void Actors::move_schedule(const ActorIndex& index)
{
	assert(m_location[index] != m_destination[index]);
	assert(m_pathIter[index] >= m_path[index].begin());
	assert(m_pathIter[index] != m_path[index].end());
	BlockIndex moveTo = *m_pathIter[index];
	m_moveEvent.schedule(index, move_delayToMoveInto(index, moveTo), m_area, index);
}
void Actors::move_setDestination(const ActorIndex& index, const BlockIndex& destination, bool detour, bool adjacent, bool unreserved, bool reserve)
{
	assert(destination.exists());
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
		m_pathRequest[index]->createGoAdjacentToLocation(m_area, index, destination, detour, unreserved, DistanceInBlocks::max(), reserve);
	else
		m_pathRequest[index]->createGoTo(m_area, index, destination, detour, unreserved, DistanceInBlocks::max(), reserve);
}
void Actors::move_setDestinationAdjacentToLocation(const ActorIndex& index, const BlockIndex& destination, bool detour, bool unreserved, bool reserve)
{
	move_setDestination(index, destination, detour, true, unreserved, reserve);
}
void Actors::move_setDestinationAdjacentToActor(const ActorIndex& index, const ActorIndex& other, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToActor(index, other));
	assert(!isAdjacentToLocation(index, m_location[other]));
	// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToActor(m_area, index, other, detour, unreserved, DistanceInBlocks::max(), reserve);
}
void Actors::move_setDestinationAdjacentToItem(const ActorIndex& index, const ItemIndex& item, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToItem(index, item));
	assert(!isAdjacentToLocation(index, m_area.getItems().getLocation(item)));
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToItem(m_area, index, item, detour, unreserved, DistanceInBlocks::max(), reserve);
}
void Actors::move_setDestinationAdjacentToPlant(const ActorIndex& index, const PlantIndex& plant, bool detour, bool unreserved, bool reserve)
{
	assert(!isAdjacentToPlant(index, plant));
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToPlant(m_area, index, plant, detour, unreserved, DistanceInBlocks::max(), reserve);
}
void Actors::move_setDestinationAdjacentToPolymorphic(const ActorIndex& index, ActorOrItemIndex actorOrItemIndex, bool detour, bool unreserved, bool reserve)
{
	assert(actorOrItemIndex.exists());
	if(actorOrItemIndex.isActor())
		move_setDestinationAdjacentToActor(index, actorOrItemIndex.getActor(), detour, unreserved, reserve);
	else
		move_setDestinationAdjacentToItem(index, actorOrItemIndex.getItem(), detour, unreserved, reserve);
}
void Actors::move_setDestinationAdjacentToFluidType(const ActorIndex& index, const FluidTypeId& fluidType, bool detour, bool unreserved, bool reserve, DistanceInBlocks maxRange)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToFluidType(m_area, index, fluidType, detour, unreserved, maxRange, reserve);
}
void Actors::move_setDestinationAdjacentToDesignation(const ActorIndex& index, const BlockDesignation& designation, bool detour, bool unreserved, bool reserve, DistanceInBlocks maxRange)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoAdjacentToDesignation(m_area, index, designation, detour, unreserved, maxRange, reserve);
}
void Actors::move_setDestinationToEdge(const ActorIndex& index, bool detour)
{
	assert(m_pathRequest[index] == nullptr);
	m_pathRequest[index] = std::make_unique<PathRequest>(PathRequest::create());
	m_pathRequest[index]->createGoToEdge(m_area, index, detour);
}
void Actors::move_setType(const ActorIndex& index, const MoveTypeId& moveType)
{
	assert(m_moveType[index] != moveType);
	m_moveType[index] = moveType;
	// TODO: unregister move type: decrement count of actors using a given facade and maybe clear when 0.
	m_area.m_hasTerrainFacades.maybeRegisterMoveType(moveType);
	//TODO: repath if destination, possibly do something with pathRequest?
}
void Actors::move_clearAllEventsAndTasks(const ActorIndex& index)
{
	m_moveEvent.maybeUnschedule(index);
	move_pathRequestMaybeCancel(index);
}
void Actors::move_onLeaveArea(const ActorIndex& index) { move_clearAllEventsAndTasks(index); }
void Actors::move_onDeath(const ActorIndex& index) { move_clearAllEventsAndTasks(index); }
bool Actors::move_tryToReserveProposedDestination(const ActorIndex& index, const BlockIndices& path)
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
bool Actors::move_tryToReserveOccupied(const ActorIndex& index)
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
bool Actors::move_destinationIsAdjacentToLocation(const ActorIndex& index, const BlockIndex& location)
{
	assert(m_destination[index].exists());
	return m_area.getBlocks().isAdjacentToIncludingCornersAndEdges(location, m_destination[index]);
}
void Actors::move_pathRequestCallback(const ActorIndex& index, BlockIndices path, bool useCurrentLocation, bool reserveDestination)
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
void Actors::move_pathRequestRecord(const ActorIndex& index, std::unique_ptr<PathRequest> pathRequest)
{
	assert(m_pathRequest[index] == nullptr);
	assert(pathRequest->exists());
	m_pathRequest[index] = std::move(pathRequest);
}
void Actors::move_updatePathRequestTerrainFacadeIndex(const ActorIndex& index, const PathRequestIndex& newPathRequestIndex)
{
	m_pathRequest[index]->update(newPathRequestIndex);
}
void Actors::move_pathRequestMaybeCancel(const ActorIndex& index)
{
	if(m_pathRequest[index] != nullptr)
	{
		m_pathRequest[index]->cancel(m_area, index);
		m_pathRequest[index] = nullptr;
	}
}
bool Actors::move_canMove(const ActorIndex& index) const
{
	if(!isAlive(index) || m_speedIndividual[index] == 0)
		return false;
	return true;
}
Step Actors::move_delayToMoveInto(const ActorIndex& index, const BlockIndex& block) const
{
	Speed speed = m_speedActual[index];
	assert(block != m_location[index]);
	Blocks& blocks = m_area.getBlocks();
	CollisionVolume staticVolume = blocks.shape_getStaticVolume(block);
	if(staticVolume > Config::minBlockStaticVolumeToSlowMovement)
	{
		CollisionVolume excessVolume = staticVolume - Config::minBlockStaticVolumeToSlowMovement;
		if(excessVolume > 50)
			excessVolume = CollisionVolume::create(50);
		speed = Speed::create(util::scaleByPercent(speed.get(), Percent::create(100 - excessVolume.get())));
	}
	assert(speed != 0);
	static const Step stepsPerSecond = Config::stepsPerSecond;
	MoveCost cost = blocks.shape_moveCostFrom(block, m_moveType[index], m_location[index]);
	assert(cost != 0);
	return Step::create(std::max(1u, uint(std::round(float(stepsPerSecond.get() * cost.get()) / float(speed.get())))));
}
Step Actors::move_stepsTillNextMoveEvent(const ActorIndex& index) const
{
	// Add 1 because we increment step number at the end of the step.
	return Step::create(1) + m_moveEvent.getStep(index) - m_area.m_simulation.m_step;
}
bool Actors::move_canPathTo(const ActorIndex& index, const BlockIndex& destination)
{
	return move_canPathFromTo(index, m_location[index], m_facing[index], destination);
}
bool Actors::move_canPathFromTo(const ActorIndex& index, const BlockIndex& start, const Facing& startFacing, const BlockIndex& destination)
{
	TerrainFacade& terrainFacade = m_area.m_hasTerrainFacades.getForMoveType(m_moveType[index]);
	return terrainFacade.accessable(start, startFacing, destination, index);
}
// MoveEvent
MoveEvent::MoveEvent(const Step& delay, Area& area, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(actor) { }
MoveEvent::MoveEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>())
{
	data["actor"].get_to(m_actor);
}
void MoveEvent::execute(Simulation&, Area* area) { area->getActors().move_callback(m_actor); }
void MoveEvent::clearReferences(Simulation &, Area *area) { area->getActors().m_moveEvent.clearPointer(m_actor); }
Json MoveEvent::toJson() const
{
	Json output = {{"delay", duration()}, {"start", m_startStep}};
	output["actor"] = m_actor;
	return output;
}
