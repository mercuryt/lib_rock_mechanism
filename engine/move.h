#pragma once
#include "config.h"
#include "deserializationMemo.h"
#include "fluidType.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "findsPath.h"

#include <vector>

class MoveEvent;
class PathThreadedTask;
class Block;
class Actor;
class MoveType;
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
public:
	ActorCanMove(Actor& a);
	ActorCanMove(const Json& data, DeserializationMemo& deserializationMemo, Actor& a);
	void updateIndividualSpeed();
	void updateActualSpeed();
	void setMoveType(const MoveType& moveType);
	void setPath(std::vector<Block*>& path);
	void clearPath();
	void callback();
	void scheduleMove();
	void setDestination(Block& destination, bool detour = false, bool adjacent = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentTo(Block& destination, bool detour = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentTo(HasShape& hasShape, bool detour = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentTo(const FluidType& fluidType, bool detour = false, bool unreserved = true, bool reserve = true);
	void clearAllEventsAndTasks();
	void onDeath();
	void onLeaveArea();
	void maybeCancelThreadedTask() { m_threadedTask.maybeCancel(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] const MoveType& getMoveType() const { return *m_moveType; }
	[[nodiscard]] uint32_t getIndividualMoveSpeedWithAddedMass(Mass mass) const;
	[[nodiscard]] uint32_t getMoveSpeed() const { return m_speedActual; }
	[[nodiscard]] bool canMove() const;
	// For testing.
	[[maybe_unused, nodiscard]] PathThreadedTask& getPathThreadedTask() { return m_threadedTask.get(); }
	[[maybe_unused, nodiscard]] std::vector<Block*>& getPath() { return m_path; }
	[[maybe_unused, nodiscard]] Block* getDestination() { return m_destination; }
	[[maybe_unused, nodiscard]] bool hasEvent() { return m_event.exists(); }
	[[maybe_unused, nodiscard]] bool hasThreadedTask() { return m_threadedTask.exists(); }
	friend class MoveEvent;
	friend class PathThreadedTask;
};
class MoveEvent final : public ScheduledEvent
{
	ActorCanMove& m_canMove;
public:
	MoveEvent(Step delay, ActorCanMove& cm, const Step start = 0);
	void execute() { m_canMove.callback(); }
	void clearReferences() { m_canMove.m_event.clearPointer(); }
};
class PathThreadedTask final : public ThreadedTask
{
	Actor& m_actor;
	HasShape* m_hasShape;
	const FluidType* m_fluidType;
	const Block* m_huristicDestination;
	bool m_detour;
	bool m_adjacent;
	bool m_unreservedDestination;
	bool m_reserveDestination;
	FindsPath m_findsPath;
public:
	PathThreadedTask(Actor& a, HasShape* hasShape = nullptr, const FluidType* fluidType = nullptr, const Block* huristicDestination = nullptr, bool detour = false, bool adjacent = false, bool m_unreservedDestination = true, bool m_reserveDestination = true);
	PathThreadedTask(const Json& data, DeserializationMemo& deserializationMemo, Actor& a);
	Json toJson() const;
	void readStep();
	void writeStep();
	void clearReferences();
	// Testing.
	[[maybe_unused]]bool isDetour() const { return m_detour; }
	[[maybe_unused]]FindsPath& getFindsPath() { return m_findsPath; }
};