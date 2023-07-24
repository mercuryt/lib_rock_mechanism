#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"

#include <vector>

class MoveEvent;
class PathThreadedTask;
class PathToSetThreadedTask;
class Block;
class Actor;
class MoveType;
class Item;
class HasShape;
class ActorCanMove
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
	HasThreadedTask<PathToSetThreadedTask> m_toSetThreadedTask;
public:
	ActorCanMove(Actor& a) : m_actor(a), m_retries(0) { }
	void updateIndividualSpeed();
	void updateActualSpeed();
	void setPath(std::vector<Block*>& path);
	void callback();
	void scheduleMove();
	void setDestination(Block& destination, bool detour = false, bool adjacent = false);
	void setDestinationAdjacentTo(Block& destination);
	void setDestinationAdjacentTo(HasShape& hasShape);
	void setDestinationAdjacentToSet(std::unordered_set<Block*>& blocks, bool detour = false);
	const MoveType& getMoveType() const;
	friend class MoveEvent;
	friend class PathThreadedTask;
	friend class PathToSetThreadedTask;
};
class MoveEvent : ScheduledEvent
{
	ActorCanMove& m_canMove;
	MoveEvent(uint32_t delay, ActorCanMove& cm) : ScheduledEvent(delay), m_canMove(cm) { }
	void execute() { m_canMove.callback(); }
	~MoveEvent() { m_canMove.m_event.clearPointer(); }
};
class PathThreadedTask : ThreadedTask
{
	Actor& m_actor;
	bool m_detour;
	bool m_adjacent;
	std::vector<Block*> m_result;
	PathThreadedTask(Actor& a, bool d = false, bool ad = false) : m_actor(a), m_detour(d), m_adjacent(ad) { }
	void readStep();
	void writeStep();
};
class PathToSetThreadedTask : ThreadedTask
{
	Actor& m_actor;
	std::unordered_set<Block*> m_blocks;
	bool m_detour;
	bool m_adjacent;
	std::vector<Block*> m_result;

	PathToSetThreadedTask(Actor& a, std::unordered_set<Block*> b, bool d = false, bool ad = false) : m_actor(a), m_blocks(b), m_detour(d), m_adjacent(ad) { }
	void readStep();
	void writeStep();
};
