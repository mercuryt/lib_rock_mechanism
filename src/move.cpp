#include "move.h"
#include "path.h"
void ActorCanMove::updateIndividualSpeed()
{
	m_speedIndividual = m_actor.m_attributes.getMoveSpeed();
	uint32_t carryMass = m_actor.m_canPickup.getMass() + m_actor.m_canPickup.getMass();
	uint32_t unencomberedCarryMass = m_actor.m_attributes.getUnencomberedCarryMass();
	if(carryMass > unencomberedCarryMass)
		m_speedIndividual = util::scaleByFraction(m_speedIndividual, unencomberedCarryMass, carryMass);
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
	if(block.m_hasShapes.canEnterCurrentlyFrom(m_actor, *m_actor.m_location))
	{
		m_retries = 0;
		m_actor.setLocation(block);
		if(++m_pathIter != m_path.end())
			scheduleMove();
	}
	else
	{
		++ m_retries;
		scheduleMove();
	}
}
void ActorCanMove::scheduleMove()
{
	uint32_t stepsToMove = m_actor.m_location->m_hasShapes.moveCostFrom(m_actor.getMoveType(), *m_actor.m_location) / m_speedActual;
	m_event.schedule(stepsToMove, *this);
}
void ActorCanMove::setDestination(Block& destination, bool detour, bool adjacent)
{
	m_destination = &destination;
	m_threadedTask.create(*this, detour, adjacent);
}
void ActorCanMove::setDestinationAdjacentToSet(std::unordered_set<Block*>& blocks, bool detour)
{
	static bool adjacent = true;
	m_toSetThreadedTask.create(*this, blocks, detour, adjacent);
}
void ActorCanMove::setDestinationAdjacentTo(HasShape& hasShape) { setDestinationAdjacentToSet(hasShape.m_blocks); }
void PathThreadedTask::readStep()
{
	m_result = path::getForActor(m_actor, *m_actor.m_canMove.m_destination);
}
void PathThreadedTask::writeStep()
{
	if(m_result.empty())
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
	{
		if(m_adjacent)
			m_result.pop_back();
		m_actor.m_canMove.setPath(m_result);
	}
}
void PathToSetThreadedTask::readStep()
{
	auto condition = [&](Block& block){ return m_blocks.contains(&block); };
	m_result = path::getForActorToPredicate(m_actor, condition);
}
void PathToSetThreadedTask::writeStep()
{
	if(m_result.empty())
		m_actor.m_hasObjectives.cannotCompleteTask();
	else
	{
		if(m_adjacent)
			m_result.pop_back();
		m_actor.m_canMove.setPath(m_result);
	}
}
