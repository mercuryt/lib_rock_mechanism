#pragma once
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "findsPath.h"

#include <vector>

class MoveEvent;
class PathThreadedTask;
class PathToSetThreadedTask;
class ExitAreaThreadedTask;
class Block;
class Actor;
struct MoveType;
class Item;
class HasShape;
class ActorCanMove final
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
	HasThreadedTask<ExitAreaThreadedTask> m_exitAreaThreadedTask;
public:
	ActorCanMove(Actor& a);
	void updateIndividualSpeed();
	void updateActualSpeed();
	void setPath(std::vector<Block*>& path);
	void clearPath();
	void callback();
	void scheduleMove();
	void setDestination(Block& destination, bool detour = false, bool adjacent = false);
	void setDestinationAdjacentTo(Block& destination, bool detour = false);
	void setDestinationAdjacentTo(HasShape& hasShape);
	void setDestinationAdjacentToSet(std::unordered_set<Block*>& blocks, bool detour = false);
	void tryToExitArea();
	void setMoveType(const MoveType& moveType);
	[[nodiscard]]const MoveType& getMoveType() const { return *m_moveType; }
	[[nodiscard]]uint32_t getIndividualMoveSpeedWithAddedMass(uint32_t mass) const;
	[[nodiscard]]uint32_t getMoveSpeed() const { return m_speedActual; }
	[[nodiscard]]bool canMove() const;
	// For testing.
	[[maybe_unused, nodiscard]]PathThreadedTask& getPathThreadedTask() { return m_threadedTask.get(); }
	[[maybe_unused, nodiscard]]std::vector<Block*>& getPath() { return m_path; }
	[[maybe_unused, nodiscard]]Block* getDestination() { return m_destination; }
	[[maybe_unused, nodiscard]]bool hasEvent() { return m_event.exists(); }
	friend class MoveEvent;
	friend class PathThreadedTask;
	friend class PathToSetThreadedTask;
	friend class ExitAreaThreadedTask;
};
class MoveEvent final : public ScheduledEventWithPercent
{
	ActorCanMove& m_canMove;
public:
	MoveEvent(Step delay, ActorCanMove& cm);
	void execute() { m_canMove.callback(); }
	void clearReferences() { m_canMove.m_event.clearPointer(); }
};
class PathThreadedTask final : public ThreadedTask
{
	Actor& m_actor;
	bool m_detour;
	bool m_adjacent;
	FindsPath m_findsPath;
public:
	PathThreadedTask(Actor& a, bool d = false, bool ad = false);
	void readStep();
	void writeStep();
	void clearReferences();
	// Testing.
	[[maybe_unused]]bool isDetour() const { return m_detour; }
	[[maybe_unused]]FindsPath& getFindsPath() { return m_findsPath; }
};
class PathToSetThreadedTask final : public ThreadedTask
{
	Actor& m_actor;
	std::unordered_set<Block*> m_blocks;
	bool m_detour;
	bool m_adjacent;
	std::vector<Block*> m_route;
	void clearReferences();
public:
	PathToSetThreadedTask(Actor& a, std::unordered_set<Block*> b, bool d = false, bool ad = false);
	void readStep();
	void writeStep();
};
class ExitAreaThreadedTask final : public ThreadedTask
{
	Actor& m_actor;
	bool m_detour;
	std::vector<Block*> m_route;
	void clearReferences();
public:
	ExitAreaThreadedTask(Actor& a, bool d);
	void readStep();
	void writeStep();
};
