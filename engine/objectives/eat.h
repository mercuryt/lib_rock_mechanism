#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"

class Area;
class EatObjective;

class EatEvent final : public ScheduledEvent
{
	EatObjective& m_eatObjective;
public:
	EatEvent(Area& area, const Step delay, EatObjective& eo, const Step start = 0);
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
};
constexpr int maxRankedEatDesire = 3;
class EatPathRequest final : public PathRequest
{
	std::array<BlockIndex, maxRankedEatDesire> m_candidates;
	EatObjective& m_eatObjective;
	ActorIndex m_huntResult = ACTOR_INDEX_MAX;
public:
	EatPathRequest(Area& area, EatObjective& eo);
	void callback(Area& area, FindPathResult result);
};
class EatObjective final : public Objective
{
	HasScheduledEvent<EatEvent> m_eatEvent;
	BlockIndex m_destination = BLOCK_INDEX_MAX;
	bool m_noFoodFound = false;
	bool m_tryToHunt = false;
public:
	EatObjective(Area& area, ActorIndex a);
	EatObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area&);
	void cancel(Area&);
	void delay(Area&);
	void reset(Area&);
	void noFoodFound();
	void makePathRequest(Area& area);
	[[nodiscard]] std::string name() const { return "eat"; }
	[[nodiscard]] bool canEatAt(Area& area, BlockIndex block) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Eat; }
	[[nodiscard]] bool isNeed() const { return true; }
	friend class EatEvent;
	friend class EatPathRequest;
	// For testing.
	[[maybe_unused, nodiscard]]bool hasEvent() const { return m_eatEvent.exists(); }
};
