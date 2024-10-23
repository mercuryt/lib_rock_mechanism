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
		static PathRequest create() { return PathRequest(); }
		PathRequest(const Json& data) { nlohmann::from_json(data, *this); }
		void updateActorIndex(const ActorIndex& newIndex) { assert(newIndex.exists()); m_actor = newIndex; }
		void create(Area& area, const ActorIndex& actor, DestinationCondition destination, bool detour, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination = BlockIndex::null(), bool reserve = false);
		void createGoToAnyOf(Area& area, const ActorIndex& actor, BlockIndices destinations, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination = BlockIndex::null(), bool reserve = false);
	public:
		void createGoTo(Area& area, const ActorIndex& actor, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve = false);
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
		void maybeCancel(Area& area, const ActorIndex& actor);
		void cancel(Area& area, const ActorIndex& actor);
		void update(const PathRequestIndex& newIndex);
		virtual void reset();
		virtual void callback(Area& area, const FindPathResult& result);
		[[nodiscard]] virtual Json toJson() const { return *this; }
		[[nodiscard]] bool exists() const { return m_index.exists(); }
		[[nodiscard]] ActorIndex getActor() const { assert(m_actor.exists()); return m_actor; }
		[[nodiscard]] std::string name() { return "basic"; }
		[[nodiscard]] static std::unique_ptr<PathRequest> load(Area& area, const Json& data, DeserializationMemo& deserializationMemo);
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
