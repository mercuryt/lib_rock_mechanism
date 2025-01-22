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
struct DeserializationMemo;

class PathRequest
	{
		PathRequestIndex m_index;
		BlockIndices m_destinations;
		FluidTypeId m_fluidType;
		ActorIndex m_actor;
		BlockIndex m_start;
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
		PathRequest() = default;
	public:
		PathRequest(const Json& data);
		void updateActorIndex(const ActorIndex& newIndex) { assert(newIndex.exists()); m_actor = newIndex; }
		void create(Area& area, const ActorIndex& actor, const BlockIndex& start, const Facing4& startFacing, DestinationCondition destination, bool detour, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination = BlockIndex::null(), bool reserve = false);
		void createGoToAnyOf(Area& area, const ActorIndex& actor, BlockIndices destinations, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination = BlockIndex::null(), bool reserve = false);
	public:
		void createGoTo(Area& area, const ActorIndex& actor, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoToFrom(Area& area, const ActorIndex& actor, const BlockIndex& start, const Facing4& startFacing, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToLocation(Area& area, const ActorIndex& actor, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToActor(Area& area, const ActorIndex& actor, const ActorIndex& other, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToItem(Area& area, const ActorIndex& actor, const ItemIndex& item, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToPlant(Area& area, const ActorIndex& actor, const PlantIndex& item, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToPolymorphic(Area& area, const ActorIndex& actor, const ActorOrItemIndex& actorOrItem, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToDesignation(Area& area, const ActorIndex& actor, const BlockDesignation& designation, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoAdjacentToFluidType(Area& area, const ActorIndex& actor, const FluidTypeId& fluidType, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
		void createGoToEdge(Area& area, const ActorIndex& actor, bool detour);
		// Multi block actors go to a destination where any of the occupied blocks fulfill the condition. This is inconsistant with GoTo which sends multi block actors to an exact tile.
		void createGoToCondition(Area& area, const ActorIndex& actor, DestinationCondition condition, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination = BlockIndex::null(), bool reserve = false);
		void createGoAdjacentToCondition(Area& area, const ActorIndex& actor, std::function<bool(const BlockIndex&)> condition, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve = false);
		void createGoAdjacentToConditionFrom(Area& area, const ActorIndex& actor, const BlockIndex& start, const Facing4& startFacing, std::function<bool(const BlockIndex&)> condition, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve = false);
		void maybeCancel(Area& area, const ActorIndex& actor);
		void cancel(Area& area, const ActorIndex& actor);
		void update(const PathRequestIndex& newIndex);
		virtual void reset();
		virtual FindPathResult findPathDepthFirst(const Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& start, const Facing4& facing, const BlockIndex& huristicDestination) = 0;
		virtual FindPathResult findPathBreadthFirst(const Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& start, const Facing4& facing) = 0;
		virtual void callback(Area& area, const FindPathResult& result);
		[[nodiscard]] virtual Json toJson() const;
		[[nodiscard]] bool exists() const { return m_index.exists(); }
		[[nodiscard]] ActorIndex getActor() const { assert(m_actor.exists()); return m_actor; }
		[[nodiscard]] std::wstring name() { return "basic"; }
		[[nodiscard]] static std::unique_ptr<PathRequest> load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
		[[nodiscard]] bool isDetour() const { return m_detour; }
		virtual ~PathRequest() = default;
};
inline void to_json(Json& data, const std::unique_ptr<PathRequest>& pathRequest) { data = pathRequest->toJson(); }
// Defines a standard callback to be subclassed by many objectives.
class ObjectivePathRequest : public PathRequest
{
protected:
	Objective& m_objective;
	bool m_reserveBlockThatPassedPredicate = false;
	ObjectivePathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	ObjectivePathRequest(Objective& objective, bool rbtpp) : m_objective(objective), m_reserveBlockThatPassedPredicate(rbtpp) { }
	void callback(Area& area, const FindPathResult&);
	virtual void onSuccess(Area& area, const BlockIndex& blockThatPassedPredicate) = 0;
	[[nodiscard]] Json toJson() const;
};
// Defines a standard callback to be subclassed by eat, sleep, drink and go to safe temperature.
class NeedPathRequest : public PathRequest
{
protected:
	Objective& m_objective;
	NeedPathRequest(Objective& objective) : m_objective(objective) { }
	NeedPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult&);
	virtual void onSuccess(Area& area, const BlockIndex& blockThatPassedPredicate) = 0;
	[[nodiscard]] Json toJson() const;
};
