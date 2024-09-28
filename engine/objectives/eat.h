#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
#include "../reference.h"

class Area;
class EatObjective;
class EatObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, ActorIndex) const { assert(false); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, ActorIndex) const { assert(false); }
	EatObjectiveType() = default;
	EatObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "eat"; }
};
class EatEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	EatObjective& m_eatObjective;
public:
	EatEvent(Area& area, Step delay, EatObjective& eo, ActorIndex actor, Step start = Step::null());
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
	void eatPreparedMeal(Area& area, ItemIndex item);
	void eatGenericItem(Area& area, ItemIndex item);
	void eatActor(Area& area, ActorIndex actor);
	void eatPlantLeaves(Area& area, PlantIndex plant);
	void eatFruitFromPlant(Area& area, PlantIndex plant);
	[[nodiscard]] BlockIndex getBlockWithMostDesiredFoodInReach(Area& area) const;
	[[nodiscard]] uint32_t getDesireToEatSomethingAt(Area& area, BlockIndex block) const;
	[[nodiscard]] uint32_t getMinimumAcceptableDesire() const;
	[[nodiscard]] std::string name() const { return "eat"; }
};
constexpr int maxRankedEatDesire = 3;
class EatPathRequest final : public PathRequest
{
	std::array<BlockIndex, maxRankedEatDesire> m_candidates;
	EatObjective& m_eatObjective;
	ActorReference m_huntResult;
public:
	EatPathRequest(Area& area, EatObjective& eo, ActorIndex actor);
	EatPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, FindPathResult& result);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "eat"; }
};
class EatObjective final : public Objective
{
	HasScheduledEvent<EatEvent> m_eatEvent;
	// Where found food item is currently.
	BlockIndex m_location;
	bool m_noFoodFound = false;
	bool m_tryToHunt = false;
public:
	EatObjective(Area& area);
	EatObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, ActorIndex actor);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex actor);
	void delay(Area&, ActorIndex actor);
	void reset(Area&, ActorIndex actor);
	void noFoodFound();
	void makePathRequest(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "eat"; }
	[[nodiscard]] bool canEatAt(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::eat; }
	friend class EatEvent;
	friend class EatPathRequest;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasEvent() const { return m_eatEvent.exists(); }
	[[maybe_unused, nodiscard]] bool hasLocation() const { return m_location.exists(); }
};
