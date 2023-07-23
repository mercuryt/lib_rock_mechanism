#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"

class Item;
class Actor;
class EatObjective;
class HungerEvent;
class Block;
class Plant;

class MustEat
{
	Actor& m_actor;
	uint32_t m_massFoodRequested;
	HasScheduledEvent<HungerEvent> m_hungerEvent;
public:
	MustEat(Actor& a) : m_actor(a), m_massFoodRequested(0), m_eatingLocation(nullptr) { }
	Block* m_eatingLocation;
	void eat(uint32_t mass);
	void setNeedsFood();
	bool needsFood() const;
	uint32_t massFoodForBodyMass() const;
	const uint32_t& getMassFoodRequested() const;
	uint32_t getPercentStarved() const;
	uint32_t getDesireToEatSomethingAt(Block& block) const;
	uint32_t getMinimumAcceptableDesire() const;
	bool canEat(Actor& actor) const;
	bool canEat(Plant& plant) const;
	bool canEat(Item& item) const;
};
class EatEvent : public ScheduledEvent
{
	EatObjective& m_eatObjective;
public:
	EatEvent(uint32_t delay, EatObjective& eo) : ScheduledEvent(delay), m_eatObjective(eo) {}
	void execute();
	void eatPreparedMeal(Item& item);
	void eatGenericItem(Item& item);
	void eatActor(Actor& actor);
	void eatPlantLeaves(Plant& plant);
	void eatFruitFromPlant(Plant& plant);
	Block* getBlockWithMostDesiredFoodInReach() const;
	uint32_t getDesireToEatSomethingAt(Block& block) const;
	u_int32_t getMinimumAcceptableDesire() const;
};
class HungerEvent : public ScheduledEvent
{
	EatObjective& m_eatObjective;
public:
	HungerEvent(uint32_t delay, EatObjective& eo);
	void execute();
};
class EatThreadedTask : ThreadedTask
{
	EatObjective& m_eatObjective;
	std::vector<Block*> m_pathResult;
	Actor* m_huntResult;
public:
	EatThreadedTask(EatObjective& eo) : m_eatObjective(eo), m_huntResult(nullptr) {}
	void readStep();
	void writeStep();
};
class EatObjective : public Objective
{
public:
	Actor& m_actor;
	HasThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	Block* m_foodLocation;
	Item* m_foodItem;
	Block* m_eatingLocation;
	bool m_noEatingLocationFound;
	EatObjective(Actor& a);
	void execute();
	bool canEatAt(Block& block);
};
