/* 
 * PathRequest:
 * Used to submit and cancel requests. Holds a PathRequestIndex which may be updated when another request in canceled.
 * Responsible for serializing path request. May be subclassed.
 */
#pragma once
#include "types.h"
#include "config.h"
#include "designations.h"
class Area;
class ActorOrItemIndex;
class Objective;
struct FluidType;
struct FindPathResult;

class PathRequest
{
	PathRequestIndex m_index = PATH_REQUEST_INDEX_MAX;
	std::vector<BlockIndex> m_destinations;
	const FluidType* m_fluidType = nullptr;
	ActorIndex m_actor = ACTOR_INDEX_MAX;
	BlockIndex m_destination = BLOCK_INDEX_MAX;
	BlockIndex m_huristicDestination = BLOCK_INDEX_MAX;
	DistanceInBlocks m_maxRange = 0;
	BlockDesignation m_designation = BlockDesignation::BLOCK_DESIGNATION_MAX;
	bool m_detour = false;
	bool m_unreserved = false;
	bool m_adjacent = false;
	bool m_reserve = false;
	bool m_reserveBlockThatPassesPredicate = false;
public:
	PathRequest() = default;
	void create(Area& area, ActorIndex actor, DestinationCondition destination, bool detour, DistanceInBlocks maxRange, BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool reserve = false);
	void createGoToAnyOf(Area& area, ActorIndex actor, std::vector<BlockIndex> destinations, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool reserve = false);
public:
	void createGoTo(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToLocation(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToItem(Area& area, ActorIndex actor, ItemIndex item, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToPlant(Area& area, PlantIndex plant, ItemIndex item, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToPolymorphic(Area& area, ActorIndex actor, ActorOrItemIndex actorOrItem, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToDesignation(Area& area, ActorIndex actor, BlockDesignation designation, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToFluidType(Area& area, ActorIndex actor, const FluidType& fluidType, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoToEdge(Area& area, ActorIndex actor, bool detour);
	// Multi block actors go to a destination where any of the occupied blocks fulfill the condition. This is inconsistant with GoTo which sends multi block actors to an exact tile.
	void createGoToCondition(Area& area, ActorIndex actor, DestinationCondition condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination = BLOCK_INDEX_MAX, bool reserve = false);
	void createGoAdjacentToCondition(Area& area, ActorIndex actor, std::function<bool(BlockIndex)> condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve = false);
	void maybeCancel(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void update(PathRequestIndex newIndex);
	virtual void reset();
	virtual void callback(Area& area, FindPathResult& result);
	[[nodiscard]] virtual Json toJson() const;
	[[nodiscard]] bool exists() const { return m_index != PATH_REQUEST_INDEX_MAX; }
	virtual ~PathRequest() = default;
};
// Defines a standard callback to be subclassed by many objectives.
class ObjectivePathRequest : public PathRequest
{
protected:
	Objective& m_objective;
	bool m_reserveBlockThatPassedPredicate = false;
	void callback(Area& area, FindPathResult);
	virtual void onSuccess(Area& area, BlockIndex blockThatPassedPredicate);

public:
	ObjectivePathRequest(Objective& objective, bool rbtpp) : m_objective(objective), m_reserveBlockThatPassedPredicate(rbtpp) { }
};
// Defines a standard callback to be subclassed by eat, sleep, drink and go to safe temperature.
class NeedPathRequest : public PathRequest
{
protected:
	Objective& m_objective;
	void callback(Area& area, FindPathResult);
	virtual void onSuccess(Area& area, BlockIndex blockThatPassedPredicate);
public:
	NeedPathRequest(Objective& objective) : m_objective(objective) { }
};
