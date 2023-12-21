#pragma once

#include "config.h"
#include "objective.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "findsPath.h"

class Item;
class Actor;
class EatObjective;
class HungerEvent;
class Block;
class Plant;

class MustEat final
{
	Actor& m_actor;
	Mass m_massFoodRequested;
	HasScheduledEvent<HungerEvent> m_hungerEvent;
	EatObjective* m_eatObjective;
public:
	MustEat(Actor& a);
	MustEat(const Json& data, Actor& a);
	Json toJson() const;
	Block* m_eatingLocation;
	void eat(Mass mass);
	void setNeedsFood();
	void onDeath();
	[[nodiscard]] bool needsFood() const;
	[[nodiscard]] Mass massFoodForBodyMass() const;
	[[nodiscard]] const Mass& getMassFoodRequested() const;
	[[nodiscard]] Percent getPercentStarved() const;
	[[nodiscard]] uint32_t getDesireToEatSomethingAt(const Block& block) const;
	[[nodiscard]] uint32_t getMinimumAcceptableDesire() const;
	[[nodiscard]] Block* getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability();
	[[nodiscard]] bool canEat(const Actor& actor) const;
	[[nodiscard]] bool canEat(const Plant& plant) const;
	[[nodiscard]] bool canEat(const Item& item) const;
	friend class HungerEvent;
	friend class EatObjective;
	// For testing.
	[[maybe_unused]]bool hasObjecive() const { return m_eatObjective != nullptr; }
	[[maybe_unused]]Step getHungerEventStep() const { return m_hungerEvent.getStep(); }
};
class EatEvent final : public ScheduledEventWithPercent
{
	EatObjective& m_eatObjective;
public:
	EatEvent(const Step delay, EatObjective& eo);
	void execute();
	void clearReferences();
	void eatPreparedMeal(Item& item);
	void eatGenericItem(Item& item);
	void eatActor(Actor& actor);
	void eatPlantLeaves(Plant& plant);
	void eatFruitFromPlant(Plant& plant);
	[[nodiscard]] Block* getBlockWithMostDesiredFoodInReach() const;
	[[nodiscard]] uint32_t getDesireToEatSomethingAt(const Block& block) const;
	[[nodiscard]] uint32_t getMinimumAcceptableDesire() const;
};
class HungerEvent final : public ScheduledEventWithPercent
{
	Actor& m_actor;
public:
	HungerEvent(const Step delay, Actor& a);
	void execute();
	void clearReferences();
};
class EatThreadedTask final : public ThreadedTask
{
	EatObjective& m_eatObjective;
	Actor* m_huntResult;
	FindsPath m_findsPath;
	bool m_noFoodFound;
public:
	EatThreadedTask(EatObjective& eo);
	void readStep();
	void writeStep();
	void clearReferences();
};
class EatObjective final : public Objective
{
	Actor& m_actor;
	HasThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	Block* m_destination;
	bool m_noFoodFound;
public:
	EatObjective(Actor& a);
	void execute();
	void cancel();
	void delay();
	void reset();
	void noFoodFound();
	[[nodiscard]] std::string name() const { return "eat"; }
	[[nodiscard]] bool canEatAt(const Block& block) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Eat; }
	[[nodiscard]] bool isNeed() const { return true; }
	friend class EatEvent;
	friend class EatThreadedTask;
	// For testing.
	[[maybe_unused, nodiscard]]bool hasEvent() const { return m_eatEvent.exists(); }
	[[maybe_unused, nodiscard]]bool hasThreadedTask() const { return m_threadedTask.exists(); }
};
