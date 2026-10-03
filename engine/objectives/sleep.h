#pragma once
#include "../path/pathRequest.h"
#include "../objective.h"
#include "../numericTypes/types.h"
#include "deserializationMemo.h"
class Area;
class SleepObjective;
class SleepPathRequest;
enum class DesireToSleepAt
{
	Impossible,
	OutsideOrOccupied,
	Inside,
	Designated,
};
NLOHMANN_JSON_SERIALIZE_ENUM(DesireToSleepAt,
{
	{ DesireToSleepAt::Impossible, "Impossible"},
	{ DesireToSleepAt::OutsideOrOccupied, "OutsideOrOccupied"},
	{ DesireToSleepAt::Inside, "Inside"},
	{ DesireToSleepAt::Designated, "Designated"},
})
class SleepObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex) const { std::unreachable(); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex) const { std::unreachable(); }
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
	void execute(Area&, ActorIndex actor) override;
	void cancel(Area&, ActorIndex actor) override;
	void delay(Area& area, ActorIndex actor) override{ cancel(area, actor); }
	void reset(Area& area, ActorIndex actor) override;
	void selectLocation(Area& area, const Point3D index, ActorIndex actor);
	void makePathRequest(Area& area, ActorIndex actor);
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("sleep").getId(); }
	[[nodiscard]] bool onCanNotPath(Area& area, ActorIndex actor);
	[[nodiscard]] DesireToSleepAt desireToSleepAt(Area& area, const Point3D point, ActorIndex actor) const;
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
	Point3D m_indoorCandidate;
	Point3D m_outdoorCandidate;
	Point3D m_maxDesireCandidate;
	bool m_sleepAtCurrentLocation = false;
public:
	SleepPathRequest(Area& area, SleepObjective& so, ActorIndex actor);
	SleepPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	std::string name() const { return "sleep"; }
	[[nodiscard]] Json toJson() const;
};
