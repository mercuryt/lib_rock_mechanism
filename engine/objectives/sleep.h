#pragma once
#include "../pathRequest.h"
#include "../objective.h"
#include "../types.h"
#include "deserializationMemo.h"
class Area;
class SleepObjective;
class SleepPathRequest;
class SleepObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, ActorIndex) const { assert(false); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, ActorIndex) const { assert(false); }
	SleepObjectiveType() = default;
	SleepObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "sleep"; }
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
	[[nodiscard]] NeedType getNeedType() const { return NeedType::sleep; }
	[[nodiscard]] Json toJson() const;
	friend class SleepPathRequest;
	friend class MustSleep;
};
// Find a place to sleep.
class SleepPathRequest final : public PathRequest
{
	SleepObjective& m_sleepObjective;
	// These variables area used from messaging from parallel code, no need to serialize.
	BlockIndex m_indoorCandidate;
	BlockIndex m_outdoorCandidate;
	BlockIndex m_maxDesireCandidate;
	bool m_sleepAtCurrentLocation = false;
public:
	SleepPathRequest(Area& area, SleepObjective& so, ActorIndex actor);
	SleepPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area&, FindPathResult& result);
	std::string name() const { return "sleep"; }
	[[nodiscard]] Json toJson() const;
};
