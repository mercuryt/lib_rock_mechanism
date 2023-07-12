#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"

#include <vector>

class MoveEvent;
class PathThreadedTask;
class Block;
class Actor;
class MoveType;
class CanMove
{
	Actor& m_actor;
	const MoveType* m_moveType;
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
	const MoveType& getMoveType() const;
	friend class MoveEvent;
	friend class PathThreadedTask;
};
class MoveEvent : ScheduledEvent
{
	CanMove& m_canMove;
	MoveEvent(uint32_t delay, CanMove& cm) : ScheduledEvent(delay), m_canMove(cm) { }
	void execute() { m_canMove.callback(); }
	~MoveEvent() { m_canMove.m_event.clearPointer(); }
};
class PathThreadedTask : ThreadedTask
{
	CanMove& m_canMove;
	bool m_detour;
	PathThreadedTask(CanMove& cm, bool d) : m_canMove(cm), m_detour(d) { }
	std::vector<Block*> m_result;
	void readStep();
	void writeStep();
};
