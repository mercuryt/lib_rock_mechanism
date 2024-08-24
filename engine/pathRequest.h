/* 
 * PathRequest:
 * Used to submit and cancel requests. Holds a PathRequestIndex which may be updated when another request in canceled.
 * Responsible for serializing path request. May be subclassed.
 */
#pragma once
#include "types.h"
#include "config.h"
#include "index.h"
#include "designations.h"
#include "callbackTypes.h"
class Area;
class ActorOrItemIndex;
class Objective;
struct FluidType;
struct FindPathResult;

class PathRequest
{
	PathRequestIndex m_index;
	std::vector<BlockIndex> m_destinations;
	FluidTypeId m_fluidType;
	ActorIndex m_actor;
	BlockIndex m_destination;
	BlockIndex m_huristicDestination;
	DistanceInBlocks m_maxRange = DistanceInBlocks::create(0);
	BlockDesignation m_designation = BlockDesignation::BLOCK_DESIGNATION_MAX;
protected:
	bool m_detour = false;
	bool m_unreserved = false;
	bool m_adjacent = false;
	bool m_reserve = false;
	bool m_reserveBlockThatPassesPredicate = false;
public:
	PathRequest() = default;
	void updateActorIndex(ActorIndex newIndex) { m_actor = newIndex; }
	void create(Area& area, ActorIndex actor, DestinationCondition destination, bool detour, DistanceInBlocks maxRange, BlockIndex huristicDestination = BlockIndex::null(), bool reserve = false);
	void createGoToAnyOf(Area& area, ActorIndex actor, std::vector<BlockIndex> destinations, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination = BlockIndex::null(), bool reserve = false);
public:
	void createGoTo(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToLocation(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToItem(Area& area, ActorIndex actor, ItemIndex item, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToPlant(Area& area, ActorIndex actor, PlantIndex item, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToPolymorphic(Area& area, ActorIndex actor, ActorOrItemIndex actorOrItem, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToDesignation(Area& area, ActorIndex actor, BlockDesignation designation, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoAdjacentToFluidType(Area& area, ActorIndex actor, FluidTypeId fluidType, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve = false);
	void createGoToEdge(Area& area, ActorIndex actor, bool detour);
	// Multi block actors go to a destination where any of the occupied blocks fulfill the condition. This is inconsistant with GoTo which sends multi block actors to an exact tile.
	void createGoToCondition(Area& area, ActorIndex actor, DestinationCondition condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination = BlockIndex::null(), bool reserve = false);
	void createGoAdjacentToCondition(Area& area, ActorIndex actor, std::function<bool(BlockIndex)> condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve = false);
	void maybeCancel(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void update(PathRequestIndex newIndex);
	virtual void reset();
	virtual void callback(Area& area, FindPathResult& result);
	[[nodiscard]] virtual Json toJson() const { return *this; }
	[[nodiscard]] bool exists() const { return m_index.exists(); }
	[[nodiscard]] ActorIndex getActor() const { return m_actor; }
	virtual ~PathRequest() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PathRequest, m_index, m_destinations, m_fluidType, m_actor, m_destination, m_huristicDestination, m_maxRange, m_designation, m_detour, m_unreserved, m_adjacent, m_reserve, m_reserveBlockThatPassesPredicate);
};
inline void to_json(Json& data, const std::unique_ptr<PathRequest>& pathRequest) { data = *pathRequest; }
// Defines a standard callback to be subclassed by many objectives.
class ObjectivePathRequest : public PathRequest
{
protected:
	Objective& m_objective;
	bool m_reserveBlockThatPassedPredicate = false;
	void callback(Area& area, FindPathResult&);
	virtual void onSuccess(Area& area, BlockIndex blockThatPassedPredicate) = 0;

public:
	ObjectivePathRequest(Objective& objective, bool rbtpp) : m_objective(objective), m_reserveBlockThatPassedPredicate(rbtpp) { }
};
// Defines a standard callback to be subclassed by eat, sleep, drink and go to safe temperature.
class NeedPathRequest : public PathRequest
{
protected:
	Objective& m_objective;
	void callback(Area& area, FindPathResult&);
	virtual void onSuccess(Area& area, BlockIndex blockThatPassedPredicate) = 0;
public:
	NeedPathRequest(Objective& objective) : m_objective(objective) { }
};
