#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
#include "../reference.h"

class Area;
class EatObjective;

class EatEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	EatObjective& m_eatObjective;
public:
	EatEvent(Area& area, Step delay, EatObjective& eo, ActorIndex actor, Step start = Step::create(0));
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
	EatPathRequest(Area& area, EatObjective& eo);
	EatPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, FindPathResult& result);
	[[nodiscard]] Json toJson() const;
};
class EatObjective final : public Objective
{
	HasScheduledEvent<EatEvent> m_eatEvent;
	BlockIndex m_destination;
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
	friend class EatEvent;
	friend class EatPathRequest;
	// For testing.
	[[maybe_unused, nodiscard]] bool hasEvent() const { return m_eatEvent.exists(); }
};
