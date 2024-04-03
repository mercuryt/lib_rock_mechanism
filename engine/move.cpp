#include "move.h"
#include "actor.h"
#include "block.h"
#include "deserializationMemo.h"
#include "moveType.h"
#include "types.h"
#include "simulation.h"
#include "util.h"
ActorCanMove::ActorCanMove(Actor& a) : m_actor(a), m_moveType(&m_actor.m_species.moveType), m_destination(nullptr), m_pathIter(m_path.end()),  m_retries(0), m_event(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine()) 
{
	updateIndividualSpeed();
}
ActorCanMove::ActorCanMove(const Json& data, DeserializationMemo& deserializationMemo, Actor& a) :
	m_actor(a),
	m_moveType(&MoveType::byName(data["moveType"].get<std::string>())),
	m_destination(data.contains("destination") ? &a.getSimulation().getBlockForJsonQuery(data["destination"]) : nullptr),
	m_pathIter(m_path.begin()), m_retries(data["retries"].get<uint8_t>()), m_event(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine())
{
	updateIndividualSpeed();
	if(data.contains("path"))
	{
		assert(!data["path"].empty());
		for(const Json& blockQuery : data["path"])
			m_path.push_back(&a.getSimulation().getBlockForJsonQuery(blockQuery));
		m_pathIter = m_path.begin() + data["pathIterOffset"].get<size_t>();
		if(data.contains("moveEventStart"))
			m_event.schedule(data["moveEventDuration"].get<Step>(), *this, data["moveEventStart"].get<Step>());
	}
	else if(data.contains("threadedTask"))
		m_threadedTask.create(data["threadedTask"], deserializationMemo, m_actor);
}
Json ActorCanMove::toJson() const
{
	Json data;
	data["moveType"] = m_moveType->name;
	if(m_destination != nullptr)
		data["destination"] = m_destination->positionToJson();
	data["retries"] = m_retries;
	if(!m_path.empty())
	{
		data["path"] = Json::array();
		for(Block* block : m_path)
			data["path"].push_back(block->positionToJson());
		data["pathIterOffset"] = m_path.begin() - m_pathIter;
	}
	if(m_event.exists())
	{
		data["moveEventStart"] = m_event.getStartStep();
		data["moveEventDuration"] = m_event.duration();
	}
	if(m_threadedTask.exists())
		data["threadedTask"] = m_threadedTask.get().toJson();
	return data;
}
uint32_t ActorCanMove::getIndividualMoveSpeedWithAddedMass(Mass mass) const
{
	uint32_t output = m_actor.m_attributes.getMoveSpeed();
	Mass carryMass = m_actor.m_equipmentSet.getMass() + m_actor.m_canPickup.getMass() + mass;
	Mass unencomberedCarryMass = m_actor.m_attributes.getUnencomberedCarryMass();
	if(carryMass > unencomberedCarryMass)
	{
		float ratio = (float)unencomberedCarryMass / (float)carryMass;
		if(ratio < Config::minimumOverloadRatio)
			return 0;
		output = std::ceil(output * ratio * ratio);
	}
	output = util::scaleByInversePercent(output, m_actor.m_body.getImpairMovePercent());
	return std::ceil(output);
}
void ActorCanMove::updateIndividualSpeed()
{
	m_speedIndividual = getIndividualMoveSpeedWithAddedMass(0);
	updateActualSpeed();
}
void ActorCanMove::updateActualSpeed()
{
	m_speedActual = m_actor.m_canLead.isLeading() ? m_actor.m_canLead.getMoveSpeed() : m_speedIndividual;
}
void ActorCanMove::setPath(std::vector<Block*>& path)
{
	assert(!path.empty());
	m_path = std::move(path);
	m_destination = m_path.back();
	m_pathIter = m_path.begin();
	clearAllEventsAndTasks();
	scheduleMove();
}
void ActorCanMove::clearPath()
{
	m_path.clear();
	m_pathIter = m_path.end();
	m_destination = nullptr;
	clearAllEventsAndTasks();
}
void ActorCanMove::callback()
{
	assert(!m_path.empty());
	assert(m_destination != nullptr);
	assert(m_pathIter >= m_path.begin());
	assert(m_pathIter != m_path.end());
	Block& block = **m_pathIter;
	// Follower is not adjacent, presumably due to being blocked, wait for them.
	if(m_actor.m_canLead.isLeading())
	{
		HasShape& follower = m_actor.m_canLead.getFollower();
		if(!m_actor.isAdjacentTo(follower) && m_actor.m_location != follower.m_location)
		{
			scheduleMove();
			return;
		}
	}
	// Path has become permanantly blocked since being generated, repath.
	if(!block.m_hasShapes.anythingCanEnterEver() || !block.m_hasShapes.canEnterEverFrom(m_actor, *m_actor.m_location))
	{
		setDestination(*m_destination, false);
		return;
	}
	if(block.m_hasShapes.canEnterCurrentlyFrom(m_actor, *m_actor.m_location))
	{
		// Path is not permanantly or temporarily blocked.
		m_retries = 0;
		m_actor.setLocation(block);
		if(&block == m_destination)
		{
			m_destination = nullptr;
			m_path.clear();
			m_actor.m_hasObjectives.taskComplete();
		}
		else
		{
			++m_pathIter;
			assert(m_pathIter != m_path.end());
			Block& nextBlock = **m_pathIter;
			if(nextBlock.m_hasShapes.anythingCanEnterEver() && block.m_hasShapes.canEnterEverFrom(m_actor, *m_actor.m_location))
				scheduleMove();
			else
				m_actor.m_hasObjectives.cannotCompleteTask();
		}
	}
	else
	{
		// Path is temporarily blocked, wait a bit and then detour if still blocked.
		if(m_retries == Config::moveTryAttemptsBeforeDetour)
		{
			if(m_actor.m_hasObjectives.hasCurrent())
				m_actor.m_hasObjectives.detour();
			else
				setDestination(*m_destination, true);
		}
		else
		{
			++m_retries;
			scheduleMove();
		}
	}
}
void ActorCanMove::scheduleMove()
{
	assert(m_actor.m_location != m_actor.m_canMove.m_destination);
	assert(m_pathIter >= m_path.begin());
	assert(m_pathIter != m_path.end());
	Block& moveTo = **m_pathIter;
	auto speed = m_speedActual;
	auto volumeAtLocationBlock = m_actor.m_shape->positions[0][3];
	assert(moveTo != *m_actor.m_location);
	if(volumeAtLocationBlock + moveTo.m_hasShapes.getTotalVolume() > Config::maxBlockVolume)
	{
		auto excessVolume = (volumeAtLocationBlock + moveTo.m_hasShapes.getStaticVolume()) - Config::maxBlockVolume;
		speed = util::scaleByInversePercent(speed, excessVolume);
	}
	Step delay = moveTo.m_hasShapes.moveCostFrom(*m_moveType, *m_actor.m_location) / speed;
	m_event.schedule(delay, *this);
}
void ActorCanMove::setDestination(Block& destination, bool detour, bool adjacent, bool unreserved, bool reserve)
{
	if(reserve)
		assert(unreserved);
	clearPath();
	// If adjacent path then destination isn't known until it's completed.
	if(!adjacent)
	{
		m_destination = &destination;
		assert(destination.m_hasShapes.anythingCanEnterEver());
		assert(m_actor.m_location != m_destination);
	}
	else
		assert(!m_actor.isAdjacentTo(destination));
	clearAllEventsAndTasks();
	if(!m_actor.getFaction())
		reserve = unreserved = false;
	if(unreserved && !adjacent)
		assert(!destination.m_reservable.isFullyReserved(m_actor.getFaction()));
	std::function<bool(const Block&)> predicate = [&](const Block& block){ return block == destination; };
	// Actor, hasShape, fluidType, destinationHuristic, detour, adjacent, unreserved.
	m_threadedTask.create(m_actor, nullptr, nullptr, &destination, detour, adjacent, unreserved, reserve);
}
void ActorCanMove::setDestinationAdjacentTo(Block& destination, bool detour, bool unreserved, bool reserve)
{
	setDestination(destination, detour, true, unreserved, reserve);
}
void ActorCanMove::setDestinationAdjacentTo(HasShape& hasShape, bool detour, bool unreserved, bool reserve) 
{
	assert(!m_actor.isAdjacentTo(hasShape));
	// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
	m_threadedTask.create(m_actor, &hasShape, nullptr, hasShape.m_location, detour, true, unreserved, reserve);
}
void ActorCanMove::setDestinationAdjacentTo(const FluidType& fluidType, bool detour, bool unreserved, bool reserve) 
{
	m_threadedTask.create(m_actor, nullptr, &fluidType, nullptr, detour, true, unreserved, reserve);
}
void ActorCanMove::setMoveType(const MoveType& moveType)
{
	assert(m_moveType != &moveType);
	m_moveType = &moveType;
	//TODO: repath if destination.
}
void ActorCanMove::clearAllEventsAndTasks()
{
	m_event.maybeUnschedule();
	m_threadedTask.maybeCancel();
}
void ActorCanMove::onLeaveArea() { clearAllEventsAndTasks(); }
void ActorCanMove::onDeath() { clearAllEventsAndTasks(); }
bool ActorCanMove::canMove() const
{
	if(!m_actor.m_alive || !m_actor.isActor() || m_speedIndividual == 0)
		return false;
	return true;
}
MoveEvent::MoveEvent(Step delay, ActorCanMove& cm, const Step start) : ScheduledEvent(cm.m_actor.getSimulation(), delay, start), m_canMove(cm) { }
// Path Threaded Task.
PathThreadedTask::PathThreadedTask(Actor& a, HasShape* hs, const FluidType* ft, const Block* hd, bool d, bool ad, bool ur, bool r) : ThreadedTask(a.getThreadedTaskEngine()), m_actor(a), m_hasShape(hs), m_fluidType(ft), m_huristicDestination(hd), m_detour(d), m_adjacent(ad), m_unreservedDestination(ur), m_reserveDestination(r), m_findsPath(a, d) 
{ 
	if(m_reserveDestination)
		assert(m_unreservedDestination);
}
PathThreadedTask::PathThreadedTask(const Json& data, DeserializationMemo& deserializationMemo, Actor& a) : ThreadedTask(a.getThreadedTaskEngine()), m_actor(a), 
	m_huristicDestination(data.contains("huristicDestination") ? &deserializationMemo.m_simulation.getBlockForJsonQuery(data["huristicDestination"]) : nullptr),
	m_detour(data["detour"].get<bool>()),
	m_adjacent(data["adjacent"].get<bool>()),
	m_unreservedDestination(data["unreservedDestination"].get<bool>()),
	m_reserveDestination(data["reserveDestination"].get<bool>()), 
	m_findsPath(m_actor, m_detour) 
{ 
	if(data.contains("hasShapeItem"))
		m_hasShape = static_cast<HasShape*>(&deserializationMemo.itemReference(data["hasShapeItem"]));
	else if(data.contains("hasShapeActor"))
		m_hasShape = static_cast<HasShape*>(&deserializationMemo.actorReference(data["hasShapeActor"]));
}
Json PathThreadedTask::toJson() const
{
	Json data({{"detour", m_detour}, {"adjacent", m_adjacent}, {"unreservedDestination", m_unreservedDestination}, {"reserveDestination", m_reserveDestination}});
	if(m_hasShape != nullptr)
	{
		if(m_hasShape->isItem())
			data["hasShapeItem"] = static_cast<Item*>(m_hasShape)->m_id;
		else
		{
			assert(m_hasShape->isActor());
			data["hasShapeActor"] = static_cast<Actor*>(m_hasShape)->m_id;
		}
	}
	if(m_huristicDestination != nullptr)
		data["huristicDestination"] = m_huristicDestination->positionToJson();
	return data;
}
void PathThreadedTask::readStep()
{
	m_findsPath.m_huristicDestination = m_huristicDestination;
	std::function<bool(const Block& block)> predicate;
	assert(m_huristicDestination != nullptr || m_hasShape != nullptr || m_fluidType != nullptr);
	if(m_hasShape != nullptr)
	{
		predicate = [&](const Block& block){ return m_hasShape->m_blocks.contains(const_cast<Block*>(&block)); };
		assert(m_adjacent);
	}
	else if(m_fluidType != nullptr)
	{
		predicate = [&](const Block& block){ return block.m_fluids.contains(m_fluidType); };
		assert(m_adjacent);
	}
	else
		predicate = [&](const Block& block){ return block == *m_huristicDestination; };

	if(m_unreservedDestination)
	{
		assert(m_actor.getFaction());
		if(m_adjacent)
			m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_actor.getFaction());
		else
			m_findsPath.pathToUnreservedPredicate(predicate, *m_actor.getFaction());
	}
	else
		if(m_adjacent)
			m_findsPath.pathToAdjacentToPredicate(predicate);
		else
			m_findsPath.pathToOccupiedPredicate(predicate);
}
void PathThreadedTask::writeStep()
{
	m_findsPath.cacheMoveCosts();
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation && (!m_unreservedDestination || m_actor.allOccupiedBlocksAreReservable(*m_actor.getFaction())))
		{
			// The actor's current location is already acceptable.
			m_actor.m_hasObjectives.taskComplete();
			return;
		}
		// TODO:
		// Should this be restart, or even cannotCompleteTask?
		// cannotFulfill will cause a delay to be appiled before this objective can be added to the list again.
		// restart or cannotCompleteTask might cause an infinite loop.
		// To prevent this we would have to know if we may reroute or not.
		// cannotFulfillObjective is the most pessamistic choice.
		// This is probably best solved by callback.
		//TODO: This conditional only exists because objectives are not mandated to always exist for all actors.
		if(m_actor.m_hasObjectives.hasCurrent())
		{
			// Objectives can handle the path not found situation directly or default to cannotFulfill.
			Objective& objective = m_actor.m_hasObjectives.getCurrent();
			if(!objective.onNoPath())
			{
				if(objective.isNeed())
					m_actor.m_hasObjectives.cannotFulfillNeed(objective);
				else
					m_actor.m_hasObjectives.cannotFulfillObjective(objective);
			}
		}
	}
	else
	{
		if(m_unreservedDestination && !m_findsPath.areAllBlocksAtDestinationReservable(m_actor.getFaction()))
		{
			// Destination is now reserved, try again.
			// Actor, hasShape, fluidType, destinationHuristic, detour, adjacent, unreserved, reserve
			m_actor.m_canMove.m_threadedTask.create(m_actor, m_hasShape, m_fluidType, m_huristicDestination, m_detour, m_adjacent, m_unreservedDestination, m_reserveDestination);
			return;
		}
		if(m_reserveDestination)
			m_findsPath.reserveBlocksAtDestination(m_actor.m_canReserve);
		m_actor.m_canMove.setPath(m_findsPath.getPath());
	}
}
void PathThreadedTask::clearReferences() { m_actor.m_canMove.m_threadedTask.clearPointer(); }
