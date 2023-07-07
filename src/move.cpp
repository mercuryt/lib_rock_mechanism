#include "move.h"
#inclued "path.h"
void CanMove::updateIndividualSpeed()
{
	m_speedIndividual = m_actor.m_attributes.getMoveSpeed();
	uint32_t carryMass = m_actor.m_hasItems.getMass() + m_actor.m_canPickup.getMass();
	uint32_t unencomberedCarryMass = m_actor.m_attributes.getUnencomberedCarryMass();
	if(carryMass > unencomberedCarryMass)
		m_speedIndividual = util::scaleByFraction(m_speedIndividual, unencomberedCarryMass, carryMass);
	setActualSpeed();
}
void CanMove::updateActualSpeed()
{
	m_speedActual = m_actor.m_canLead.isLeading() ? m_actor.m_canLead.getMoveSpeed() : m_speedIndividual;
}
void CanMove::setPath(std::vector<Block*>& path)
{
	m_path = path;
	m_destination = path.back();
	m_pathIter = path.begin();
	scheduleMove();
}
void CanMove::callback()
{
	Block& block = **m_pathIter;
	if(block.m_hasActors.actorCanEnterCurrently(m_actor))
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
void CanMove::scheduleMove()
{
	uint32_t stepsToMove = block->m_hasActors.moveCost(*actor.m_moveType, *actor.m_location) / m_speedActual;
	m_event.schedule(stepsToMove, *this);
}
void CanMove::setDestination(Block* destination, bool detour = false)
{
	m_destination = destination;
	m_threadedTask.create(*this, detour);
}
void CanMove::readStep()
{
	m_result = path::getForActor(m_canMove.m_actor, end, m_detour);
}
void CanMove::writeStep()
{
	if(m_result.empty())
		m_canMove.m_actor.m_hasObjectives.cannotCompleteTask();
	else
		m_canMove.setPath(m_result);
}
