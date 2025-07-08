#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../numericTypes/types.h"
#include "../reference.h"

class Area;
class EatObjective;
class EatObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { std::unreachable(); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { std::unreachable(); }
	EatObjectiveType() = default;
	EatObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "eat"; }
};
class EatEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	EatObjective& m_eatObjective;
public:
	EatEvent(Area& area, const Step& delay, EatObjective& eo, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
	void eatPreparedMeal(Area& area, const ItemIndex& item);
	void eatGenericItem(Area& area, const ItemIndex& item);
	void eatActor(Area& area, const ActorIndex& actor);
	void eatPlantLeaves(Area& area, const PlantIndex& plant);
	void eatFruitFromPlant(Area& area, const PlantIndex& plant);
	[[nodiscard]] uint32_t getDesireToEatSomethingAt(Area& area, const Point3D& point) const;
	[[nodiscard]] uint32_t getMinimumAcceptableDesire() const;
	[[nodiscard]] std::string name() const { return "eat"; }
};
constexpr int maxRankedEatDesire = 3;
class EatPathRequest final : public PathRequestBreadthFirst
{
	std::array<Point3D, maxRankedEatDesire> m_candidates;
	EatObjective& m_eatObjective;
	ActorReference m_huntResult;
public:
	EatPathRequest(Area& area, EatObjective& eo, const ActorIndex& actor);
	EatPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "eat"; }
};
class EatObjective final : public Objective
{
	HasScheduledEvent<EatEvent> m_eatEvent;
	// Where found food item is currently.
	Point3D m_location;
	bool m_noFoodFound = false;
	bool m_tryToHunt = false;
public:
	EatObjective(Area& area);
	EatObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const ActorIndex& actor);
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area&, const ActorIndex& actor);
	void reset(Area&, const ActorIndex& actor);
	void noFoodFound();
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "eat"; }
	[[nodiscard]] bool canEatAt(Area& area, const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::eat; }
	friend class EatEvent;
	friend class EatPathRequest;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasEvent() const { return m_eatEvent.exists(); }
	[[maybe_unused, nodiscard]] bool hasLocation() const { return m_location.exists(); }
};
