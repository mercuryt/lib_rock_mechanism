#include "move.h"
#include "actor.h"
#include "block.h"
#include "util.h"
ActorCanMove::ActorCanMove(Actor& a) : m_actor(a), m_moveType(&m_actor.m_species.moveType), m_destination(nullptr), m_pathIter(m_path.end()),  m_retries(0), m_event(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine()) 
{
	updateIndividualSpeed();
}
uint32_t ActorCanMove::getIndividualMoveSpeedWithAddedMass(Mass mass) const
{
	uint32_t output = m_actor.m_attributes.getMoveSpeed();
	Mass carryMass = m_actor.m_equipmentSet.getMass() + m_actor.m_canPickup.getMass() + mass;
	Mass unencomberedCarryMass = m_actor.m_attributes.getUnencomberedCarryMass();
	if(carryMass > unencomberedCarryMass)
		output = util::scaleByFraction(m_speedIndividual, unencomberedCarryMass, carryMass);
	output = util::scaleByInversePercent(output, m_actor.m_body.getImpairMovePercent());
	return output;
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
}
void ActorCanMove::callback()
{
	assert(!m_path.empty());
	assert(m_destination != nullptr);
	assert(m_pathIter >= m_path.begin());
	assert(m_pathIter != m_path.end());
	Block& block = **m_pathIter;
	// Path has become permanantly blocked since being generated, repath.
	if(!block.m_hasShapes.anythingCanEnterEver() || !block.m_hasShapes.canEnterEverFrom(m_actor, *m_actor.m_location))
	{
		setDestination(*m_destination);
		return;
	}
	// Follower is not adjacent, presumably due to being blocked, wait for them.
	if(m_actor.m_canLead.isLeading() && !m_actor.isAdjacentTo(m_actor.m_canLead.getFollower()))
	{
		scheduleMove();
		return;
	}
	// Path is not blocked.
	if(block.m_hasShapes.canEnterCurrentlyFrom(m_actor, *m_actor.m_location))
	{
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
			scheduleMove();
		}
	}
	else
	{
		// Path is temporarily blocked, wait a bit and then detour if still blocked.
		if(m_retries == Config::moveTryAttemptsBeforeDetour)
			setDestination(*m_destination, true);
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
	assert(destination.m_hasShapes.anythingCanEnterEver());
	clearPath();
	m_destination = &destination;
	clearAllEventsAndTasks();
	std::function<bool(const Block&)> predicate = [&](const Block& block){ return block == destination; };
	// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
	m_threadedTask.create(m_actor, predicate, &destination, detour, adjacent, unreserved, reserve);
}
void ActorCanMove::setDestinationAdjacentTo(Block& destination, bool detour, bool unreserved, bool reserve)
{
	assert(!m_actor.isAdjacentTo(destination));
	setDestination(destination, detour, true, unreserved, reserve);
}
void ActorCanMove::setDestinationAdjacentTo(HasShape& hasShape, bool detour, bool unreserved, bool reserve) 
{
	std::function<bool(const Block&)> predicate = [&](const Block& block){ return hasShape.m_blocks.contains(const_cast<Block*>(&block)); };
	// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
	m_threadedTask.create(m_actor, predicate, hasShape.m_location, detour, true, unreserved, reserve);
}
void ActorCanMove::goToPredicateBlockAndThen(std::function<bool(const Block&)>& predicate, std::function<void(Block&)> callback, bool detour, bool adjacent, bool unreserved, bool reserve)
{
	Block* block = adjacent ?
		m_actor.getBlockWhichIsAdjacentWithPredicate(predicate) :
		m_actor.getBlockWhichIsOccupiedWithPredicate(predicate);
	if(block != nullptr)
		callback(*block);
	else
	{
		const Block* huristicDestination = nullptr;
		m_threadedTask.create(m_actor, predicate, huristicDestination, detour, adjacent, unreserved, reserve);
	}
}
void ActorCanMove::goToBlockAndThen(const Block& block, std::function<void(Block&)> callback, bool detour, bool adjacent, bool unreserved, bool reserve)
{
	std::function<bool(const Block&)> predicate = [&](const Block& other){ return block == other; };
	Block* target = adjacent ?
		m_actor.getBlockWhichIsAdjacentWithPredicate(predicate) :
		m_actor.getBlockWhichIsOccupiedWithPredicate(predicate);
	if(target != nullptr)
		callback(*target);
	else
	{
		const Block* huristicDestination = &block;
		m_threadedTask.create(m_actor, predicate, huristicDestination, detour, adjacent, unreserved, reserve);
	}
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
MoveEvent::MoveEvent(Step delay, ActorCanMove& cm) : ScheduledEventWithPercent(cm.m_actor.getSimulation(), delay), m_canMove(cm) { }
PathThreadedTask::PathThreadedTask(Actor& a, std::function<bool(const Block&)>& p, const Block* hd, bool d, bool ad, bool ur, bool res) : ThreadedTask(a.getThreadedTaskEngine()), m_actor(a), m_predicate(p), m_huristicDestination(hd), m_detour(d), m_adjacent(ad), m_unreserved(ur), m_reserve(res), m_findsPath(a) { }
void PathThreadedTask::readStep()
{
	m_findsPath.m_detour = m_detour;
	m_findsPath.m_huristicDestination = m_huristicDestination;
	if(m_unreserved)
	{
		if(m_adjacent)
			m_findsPath.pathToUnreservedAdjacentToPredicate(m_predicate, *m_actor.getFaction());
		else
			m_findsPath.pathToUnreservedPredicate(m_predicate, *m_actor.getFaction());
	}
	else
		if(m_adjacent)
			m_findsPath.pathToAdjacentToPredicate(m_predicate);
		else
			m_findsPath.pathToOccupiedPredicate(m_predicate);
}
void PathThreadedTask::writeStep()
{
	m_findsPath.cacheMoveCosts();
	if(!m_findsPath.found())
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
	{
		if(m_unreserved && (!m_actor.allBlocksAtLocationAndFacingAreReservable(*m_findsPath.getPath().back(), m_findsPath.getFacingAtDestination()) || m_findsPath.m_target->m_reservable.isFullyReserved(m_actor.getFaction())))
		{
			// Destination is now reserved, try again.
			// Actor, predicate, destinationHuristic, detour, adjacent, unreserved.
			m_actor.m_canMove.m_threadedTask.create(m_actor, m_predicate, m_huristicDestination, m_detour, m_adjacent, m_unreserved, m_reserve);
			return;
		}
		if(m_reserve)
		{
			assert(m_unreserved);
			m_actor.m_canReserve.clearAll(); // Somewhat skeptical about this.
			m_findsPath.reserveBlocksAtDestination(m_actor.m_canReserve);
			m_findsPath.m_target->m_reservable.reserveFor(m_actor.m_canReserve, 1u);
		}
		m_actor.m_canMove.setPath(m_findsPath.getPath());
	}
}
void PathThreadedTask::clearReferences() { m_actor.m_canMove.m_threadedTask.clearPointer(); }
