#pragma once
#include "../path/pathRequest.h"
#include "../objective.h"
#include "../types.h"
#include "deserializationMemo.h"
class Area;
class SleepObjective;
class SleepPathRequest;
class SleepObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { assert(false); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { assert(false); }
	SleepObjectiveType() = default;
	SleepObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::wstring name() const { return L"sleep"; }
};
class SleepObjective final : public Objective
{
	bool m_noWhereToSleepFound = false;
public:
	SleepObjective();
	SleepObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	void selectLocation(Area& area, const BlockIndex& index, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] bool onCanNotPath(Area& area, const ActorIndex& actor);
	[[nodiscard]] uint32_t desireToSleepAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	[[nodiscard]] std::wstring name() const { return L"sleep"; }
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::sleep; }
	[[nodiscard]] Json toJson() const;
	friend class SleepPathRequest;
	friend class MustSleep;
};
// Find a place to sleep.
class SleepPathRequest final : public PathRequestBreadthFirst
{
	SleepObjective& m_sleepObjective;
	// These variables area used from messaging from parallel code, no need to serialize.
	BlockIndex m_indoorCandidate;
	BlockIndex m_outdoorCandidate;
	BlockIndex m_maxDesireCandidate;
	bool m_sleepAtCurrentLocation = false;
public:
	SleepPathRequest(Area& area, SleepObjective& so, const ActorIndex& actor);
	SleepPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	std::wstring name() const { return L"sleep"; }
	[[nodiscard]] Json toJson() const;
};
