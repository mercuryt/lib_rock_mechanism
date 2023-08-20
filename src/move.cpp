#include "move.h"
#include "path.h"
ActorCanMove::ActorCanMove(Actor& a) : m_actor(a), m_moveType(&m_actor.m_species.moveType), m_retries(0)
{
	updateIndividualSpeed();
}
uint32_t ActorCanMove::getIndividualMoveSpeedWithAddedMass(uint32_t mass) const
{
	uint32_t output = m_actor.m_attributes.getMoveSpeed();
	uint32_t carryMass = m_actor.m_equipmentSet.getMass() + m_actor.m_canPickup.getMass() + mass;
	uint32_t unencomberedCarryMass = m_actor.m_attributes.getUnencomberedCarryMass();
	if(carryMass > unencomberedCarryMass)
		output = util::scaleByFraction(m_speedIndividual, unencomberedCarryMass, carryMass);
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
	m_path = path;
	m_destination = path.back();
	m_pathIter = path.begin();
	scheduleMove();
}
void ActorCanMove::callback()
{
	Block& block = **m_pathIter;
	// Path has become permanantly blocked since being generated, repath.
	if(!block.m_hasShapes.anythingCanEnterEver() || !block.m_hasShapes.canEnterEverFrom(m_actor, *m_actor.m_location))
	{
		setDestination(*m_destination);
		return;
	}
	// Path is temporarily blocked, wait a bit and then detour if still blocked.
	if(block.m_hasShapes.canEnterCurrentlyFrom(m_actor, *m_actor.m_location))
	{
		m_retries = 0;
		m_actor.setLocation(block);
		if(&block == m_actor.m_canMove.m_destination)
		{
			m_destination = nullptr;
			m_path.clear();
			m_actor.m_hasObjectives.taskComplete();
		}
		else
		{
			++m_pathIter;
			scheduleMove();
		}
	}
	else
	{
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
	Step delay = m_actor.m_location->m_hasShapes.moveCostFrom(*m_moveType, *m_actor.m_location) / m_speedActual;
	m_event.schedule(delay, *this);
}
void ActorCanMove::setDestination(Block& destination, bool detour, bool adjacent)
{
	m_destination = &destination;
	m_threadedTask.create(m_actor, detour, adjacent);
}
void ActorCanMove::setDestinationAdjacentTo(Block& destination, bool detour)
{
	setDestination(destination, detour, true);
}
void ActorCanMove::setDestinationAdjacentToSet(std::unordered_set<Block*>& blocks, bool detour)
{
	static bool adjacent = true;
	m_toSetThreadedTask.create(m_actor, blocks, detour, adjacent);
}
void ActorCanMove::setDestinationAdjacentTo(HasShape& hasShape) { setDestinationAdjacentToSet(hasShape.m_blocks); }
void ActorCanMove::tryToExitArea() { m_exitAreaThreadedTask.create(m_actor, false); }
void ActorCanMove::setMoveType(const MoveType& moveType)
{
	assert(m_moveType != &moveType);
	m_moveType = &moveType;
	//TODO: repath if destination.
}
bool ActorCanMove::canMove() const
{
	if(!m_actor.m_alive || !m_actor.isActor() || m_speedIndividual == 0)
		return false;
	return true;
}
void PathThreadedTask::readStep()
{
	pathTo(m_actor, *m_actor.m_canMove.m_destination, m_detour);
}
void PathThreadedTask::writeStep()
{
	if(m_route.empty())
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
	{
		if(m_adjacent)
			m_route.pop_back();
		m_actor.m_canMove.setPath(m_route);
	}
	cacheMoveCosts(m_actor);
}
void PathThreadedTask::clearReferences() { m_actor.m_canMove.m_threadedTask.clearPointer(); }
void PathToSetThreadedTask::readStep()
{
	auto condition = [&](Block& block){ return m_blocks.contains(&block); };
	m_route = path::getForActorToPredicate(m_actor, condition);
}
void PathToSetThreadedTask::writeStep()
{
	if(m_route.empty())
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
	{
		if(m_adjacent)
			m_route.pop_back();
		m_actor.m_canMove.setPath(m_route);
	}
}
void PathToSetThreadedTask::clearReferences() { m_actor.m_canMove.m_toSetThreadedTask.clearPointer(); }
void ExitAreaThreadedTask::readStep()
{
	auto condition = [&](Block& block) { return block.m_isEdge; };
	m_route = path::getForActorToPredicate(m_actor, condition);
}
void ExitAreaThreadedTask::writeStep()
{
	if(m_route.empty())
		m_actor.m_hasObjectives.wait(Config::stepsToWaitWhenCannotExitArea);
	else
		m_actor.m_canMove.setPath(m_route);
}
void ExitAreaThreadedTask::clearReferences() { m_actor.m_canMove.m_exitAreaThreadedTask.clearPointer(); }
