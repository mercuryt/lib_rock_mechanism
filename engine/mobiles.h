/*
 * A non virtual base class for Actor, and potentaially vehicle.
 */
#pragma once
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "portables.h"
#include "bloomHashMap.h"
#include "types.h"

struct MoveType;
struct FluidType;
class MoveEvent;
class OnDestroy;
class Reservable;
class CanLead;
class CanFollow;
class Area;
class PathThreadedTask;

class Mobiles : public Portables
{
protected:
	HasScheduledEvents<MoveEvent> m_moveEvent;
	HasThreadedTasks<PathThreadedTask> m_threadedTask;
	std::vector<std::vector<BlockIndex>> m_path;
	std::vector<std::vector<BlockIndex>::iterator> m_pathIter;
	std::vector<BlockIndex> m_destination;
	std::vector<Speed> m_speedIndividual;
	std::vector<Speed> m_speedActual;
	std::vector<uint8_t> m_moveRetries;
public:
	Mobiles(Area& area);
	Mobiles(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	HasShapeIndex extend();
	[[nodiscard]] HasShapeIndex create(const MoveType& moveType, const Shape& shape, BlockIndex location, Facing facing, bool isStatic);
	void destroy(HasShapeIndex index);
	void createEmptyReservable(HasShapeIndex index);
	void createEmptyOnDestroy(HasShapeIndex index);
	void log(HasShapeIndex index) const;
	void updateIndividualSpeed(HasShapeIndex index);
	void updateActualSpeed(HasShapeIndex index);
	void setMoveType(HasShapeIndex index, const MoveType& moveType);
	void setPath(HasShapeIndex index, std::vector<BlockIndex>& path);
	void clearPath(HasShapeIndex index);
	void callback(HasShapeIndex index);
	void scheduleMove(HasShapeIndex index);
	void setDestination(HasShapeIndex index, BlockIndex destination, bool detour = false, bool adjacent = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentToLocation(HasShapeIndex index, BlockIndex destination, bool detour = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentToItem(HasShapeIndex index, ItemIndex item, bool detour = false, bool unreserved = true, bool reserve = true);
	void setDestinationAdjacentToFluid(HasShapeIndex index, const FluidType& fluidType, bool detour = false, bool unreserved = true, bool reserve = true);
	void clearAllEventsAndTasks(HasShapeIndex index);
	void onLeaveArea(HasShapeIndex index);
	void maybeCancelThreadedTask(HasShapeIndex index) { m_threadedTask.maybeCancel(index); }
	[[nodiscard]] Json toJson(HasShapeIndex index) const;
	[[nodiscard]] const MoveType& getMoveType(HasShapeIndex index) const { return *m_moveType.at(index); }
	[[nodiscard]] Speed getMoveSpeed(HasShapeIndex index) const { return m_speedActual.at(index); }
	[[nodiscard]] bool canMove(HasShapeIndex index) const;
	[[nodiscard]] Step delayToMoveInto(HasShapeIndex index, const BlockIndex block) const;
	// For testing.
	[[maybe_unused, nodiscard]] PathThreadedTask& getPathThreadedTask(HasShapeIndex index) { return m_threadedTask.get(index); }
	[[maybe_unused, nodiscard]] std::vector<BlockIndex>& getPath(HasShapeIndex index) { return m_path.at(index); }
	[[maybe_unused, nodiscard]] BlockIndex getDestination(HasShapeIndex index) { return m_destination.at(index); }
	[[maybe_unused, nodiscard]] bool hasEvent(HasShapeIndex index) const { return m_moveEvent.exists(index); }
	[[maybe_unused, nodiscard]] bool hasThreadedTask(HasShapeIndex index) const { return m_threadedTask.exists(index); }
	[[maybe_unused, nodiscard]] Step stepsTillNextMoveEvent(HasShapeIndex index) const;
};
