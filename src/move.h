#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"
#include "block.h"
#include <vector>
class MoveEvent;
class PathThreadedTask;
class CanMove
{
	Actor& m_actor;
	uint32_t m_speedIndividual;
	uint32_t m_speedActual;
	Block* m_destination;
	std::vector<Block*> m_path;
	std::vector<Block*>::iterator m_pathIter;
	uint8_t m_retries;
	HasScheduledEvent<MoveEvent> m_event;
	HasThreadedTask<PathThreadedTask> m_threadedTask;
public:
	CanMove(Actor& a) : m_actor(a), m_retries(0) { }
	void updateIndividualSpeed();
	void updateActualSpeed();
	void setPath(std::vector<Block*>& path);
	void callback();
	void scheduleMove();
	void setDestination(Block* destination, bool detour = false);
};
class MoveEvent : ScheduledEvent
{
	CanMove& m_canMove;
	MoveEvent(uint32_t delay, CanMove& cm) : ScheduledEvent(delay), m_canMove(cm);
	void execute() { m_canMove.callback(); }
	~MoveEvent() { m_canMove.m_event.clearPointer(); }
};
class PathThreadedTask : ThreadedTask
{
	CanMove& m_canMove;
	bool m_detour;
	PathThreadedTask(CanMove& cm, bool d) : m_canMove(cm), m_detour(d) { }
	std::vector<Block*> m_result;
	void readStep()
	{
		m_result = path::getForActor(m_canMove.m_actor, end, m_detour);
	}
	void writeStep()
	{
		if(m_result.empty())
			m_canMove.m_actor.m_hasObjectives.cannotCompleteTask();
		else
			m_canMove.setPath(m_result);
	}
};
