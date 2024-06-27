#pragma once
#include "../pathRequest.h"
#include "../objective.h"
#include "../types.h"
class Area;
class SleepObjective;

// Find a place to sleep.
class SleepPathRequest final : public PathRequest
{
	SleepObjective& m_sleepObjective;
	BlockIndex m_indoorCandidate = BLOCK_INDEX_MAX;
	BlockIndex m_outdoorCandidate = BLOCK_INDEX_MAX;
	BlockIndex m_maxDesireCandidate = BLOCK_INDEX_MAX;
	bool m_sleepAtCurrentLocation = false;
public:
	SleepPathRequest(Area& area, SleepObjective& so);
	void callback(Area&, FindPathResult result);
};
class SleepObjective final : public Objective
{
	ActorIndex m_actor = ACTOR_INDEX_MAX;
	bool m_noWhereToSleepFound = false;
public:
	SleepObjective(ActorIndex a);
	SleepObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area&);
	void cancel(Area&);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	void selectLocation(Area& area, BlockIndex index);
	void makePathRequest(Area& area);
	[[nodiscard]] bool onCanNotRepath(Area& area);
	[[nodiscard]] uint32_t desireToSleepAt(Area& area, BlockIndex block) const;
	[[nodiscard]] std::string name() const { return "sleep"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Sleep; }
	[[nodiscard]] bool isNeed() const { return true; }
	friend class SleepPathRequest;
	friend class MustSleep;
};
