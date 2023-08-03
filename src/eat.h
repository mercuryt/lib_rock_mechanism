#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "eventSchedule.hpp"

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
class EatEvent : public ScheduledEventWithPercent
{
	EatObjective& m_eatObjective;
public:
	EatEvent(uint32_t delay, EatObjective& eo) : ScheduledEventWithPercent(delay), m_eatObjective(eo) { }
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
class HungerEvent : public ScheduledEventWithPercent
{
	Actor& m_actor;
public:
	HungerEvent(uint32_t delay, Actor& a) : ScheduledEventWithPercent(delay), m_actor(a) { }
	void execute();
};
class EatThreadedTask : public ThreadedTask
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
	Actor& m_actor;
	HasThreadedTask<EatThreadedTask> m_threadedTask;
	HasScheduledEvent<EatEvent> m_eatEvent;
	Block* m_foodLocation;
	Item* m_foodItem;
	Block* m_eatingLocation;
	bool m_noEatingLocationFound;
public:
	EatObjective(Actor& a);
	void execute();
	void cancel() {}
	bool canEatAt(Block& block) const;
	friend class EatEvent;
	friend class EatThreadedTask;
};
