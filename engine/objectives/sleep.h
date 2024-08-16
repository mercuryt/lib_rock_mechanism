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
	BlockIndex m_indoorCandidate;
	BlockIndex m_outdoorCandidate;
	BlockIndex m_maxDesireCandidate;
	bool m_sleepAtCurrentLocation = false;
public:
	SleepPathRequest(Area& area, SleepObjective& so);
	void callback(Area&, FindPathResult& result);
};
class SleepObjective final : public Objective
{
	bool m_noWhereToSleepFound = false;
public:
	SleepObjective();
	SleepObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	void selectLocation(Area& area, BlockIndex index, ActorIndex actor);
	void makePathRequest(Area& area, ActorIndex actor);
	[[nodiscard]] bool onCanNotRepath(Area& area, ActorIndex actor);
	[[nodiscard]] uint32_t desireToSleepAt(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] std::string name() const { return "sleep"; }
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] Json toJson() const;
	friend class SleepPathRequest;
	friend class MustSleep;
};
