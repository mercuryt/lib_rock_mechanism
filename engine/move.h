#pragma once
#include "config.h"
#include "fluidType.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "findsPath.h"
#include "types.h"

#include <vector>

class MoveEvent;
class PathThreadedTask;
class Actor;
class MoveType;
class Item;
class HasShape;
struct DeserializationMemo;
class Simulation;

class ActorCanMove final
{
	std::vector<BlockIndex> m_path;
	std::vector<BlockIndex>::iterator m_pathIter;
	HasScheduledEvent<MoveEvent> m_event;
	HasThreadedTask<PathThreadedTask> m_threadedTask;
	Actor& m_actor;
	const MoveType* m_moveType = nullptr;
	BlockIndex m_destination = BLOCK_INDEX_MAX;
	Speed m_speedIndividual = 0;
	Speed m_speedActual = 0;
	uint8_t m_retries = 0;
public:
	ActorCanMove(Actor& a, Simulation& s);
	ActorCanMove(const Json& data, DeserializationMemo& deserializationMemo, Actor& a);
	void updateIndividualSpeed();
	void updateActualSpeed();
	void setMoveType(const MoveType& moveType);
	void setPath(std::vector<BlockIndex>& path);
	void clearPath();
	void callback();
	void scheduleMove();
	void setDestination(BlockIndex destination, bool detour = false, bool adjacent = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentTo(BlockIndex destination, bool detour = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentTo(HasShape& hasShape, bool detour = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentTo(const FluidType& fluidType, bool detour = false, bool unreserved = true, bool reserve = true);
	void clearAllEventsAndTasks();
	void onDeath();
	void onLeaveArea();
	void maybeCancelThreadedTask() { m_threadedTask.maybeCancel(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] const MoveType& getMoveType() const { return *m_moveType; }
	[[nodiscard]] Speed getIndividualMoveSpeedWithAddedMass(Mass mass) const;
	[[nodiscard]] Speed getMoveSpeed() const { return m_speedActual; }
	[[nodiscard]] bool canMove() const;
	[[nodiscard]] Step delayToMoveInto(const BlockIndex block) const;
	// For testing.
	[[maybe_unused, nodiscard]] PathThreadedTask& getPathThreadedTask() { return m_threadedTask.get(); }
	[[maybe_unused, nodiscard]] std::vector<BlockIndex>& getPath() { return m_path; }
	[[maybe_unused, nodiscard]] BlockIndex getDestination() { return m_destination; }
	[[maybe_unused, nodiscard]] bool hasEvent() const { return m_event.exists(); }
	[[maybe_unused, nodiscard]] bool hasThreadedTask() const { return m_threadedTask.exists(); }
	[[maybe_unused, nodiscard]] Step stepsTillNextMoveEvent() const;
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
	const BlockIndex m_huristicDestination;
	bool m_detour;
	bool m_adjacent;
	bool m_unreservedDestination;
	bool m_reserveDestination;
	FindsPath m_findsPath;
public:
	PathThreadedTask(Actor& a, HasShape* hasShape = nullptr, const FluidType* fluidType = nullptr, const BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool detour = false, bool adjacent = false, bool m_unreservedDestination = true, bool m_reserveDestination = true);
	PathThreadedTask(const Json& data, DeserializationMemo& deserializationMemo, Actor& a);
	Json toJson() const;
	void readStep();
	void writeStep();
	void clearReferences();
	// Testing.
	[[maybe_unused]] bool isDetour() const { return m_detour; }
	[[maybe_unused]] FindsPath& getFindsPath() { return m_findsPath; }
};
