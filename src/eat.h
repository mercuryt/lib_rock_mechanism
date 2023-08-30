#pragma once

#include "objective.h"
#include "threadedTask.hpp"
#include "eventSchedule.hpp"
#include "pathToBlockBaseThreadedTask.h"

class Item;
class Actor;
class EatObjective;
class HungerEvent;
class Block;
class Plant;

class MustEat final
{
	Actor& m_actor;
	uint32_t m_massFoodRequested;
	HasScheduledEvent<HungerEvent> m_hungerEvent;
public:
	MustEat(Actor& a);
	Block* m_eatingLocation;
	void eat(uint32_t mass);
	void setNeedsFood();
	bool needsFood() const;
	uint32_t massFoodForBodyMass() const;
	const uint32_t& getMassFoodRequested() const;
	uint32_t getPercentStarved() const;
	uint32_t getDesireToEatSomethingAt(Block& block) const;
	uint32_t getMinimumAcceptableDesire() const;
	bool canEat(const Actor& actor) const;
	bool canEat(const Plant& plant) const;
	bool canEat(const Item& item) const;
	friend class HungerEvent;
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
class EatThreadedTask final : public PathToBlockBaseThreadedTask
{
	EatObjective& m_eatObjective;
	Actor* m_huntResult;
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
	Block* m_eatingLocation;
	bool m_noEatingLocationFound;
public:
	EatObjective(Actor& a);
	void execute();
	void cancel();
	std::string name() { return "eat"; }
	bool canEatAt(const Block& block) const;
	friend class EatEvent;
	friend class EatThreadedTask;
};
