#pragma once

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
	Block* m_eatingLocation;
	void eat(Mass mass);
	void setNeedsFood();
	void onDeath();
	bool needsFood() const;
	Mass massFoodForBodyMass() const;
	const Mass& getMassFoodRequested() const;
	Percent getPercentStarved() const;
	uint32_t getDesireToEatSomethingAt(const Block& block) const;
	uint32_t getMinimumAcceptableDesire() const;
	bool canEat(const Actor& actor) const;
	bool canEat(const Plant& plant) const;
	bool canEat(const Item& item) const;
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
	Block* getBlockWithMostDesiredFoodInReach() const;
	uint32_t getDesireToEatSomethingAt(const Block& block) const;
	u_int32_t getMinimumAcceptableDesire() const;
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
	Block* m_foodLocation;
	Item* m_foodItem;
	bool m_noFoodFound;
public:
	EatObjective(Actor& a);
	void execute();
	void cancel();
	void delay();
	std::string name() const { return "eat"; }
	bool canEatAt(const Block& block) const;
	ObjectiveId getObjectiveId() const { return ObjectiveId::Eat; }
	friend class EatEvent;
	friend class EatThreadedTask;
	// For testing.
	[[maybe_unused, nodiscard]]bool hasEvent() const { return m_eatEvent.exists(); }
	[[maybe_unused, nodiscard]]bool hasThreadedTask() const { return m_threadedTask.exists(); }
};
