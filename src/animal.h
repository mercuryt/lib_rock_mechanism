#pragma once
#include "eventSchedule.h"

template<class FluidType>
struct BaseAnimalType
{
	const std::string name;
	const uint32_t minimumTemperature;
	const uint32_t maximumTemperature;
	const uint32_t stepsTillDieTemperature;
	const FluidType& fluidType;
	const uint32_t stepsNeedsFluidFrequency;
	const uint32_t stepsTillDieWithoutFluid;
	const bool carnivore;
	const bool herbavore;
	const uint32_t stepsNeedsFoodFrequency;
	const uint32_t stepsTillDieWithoutFood;
	const uint32_t stepsTillFullyGrown;
	const uint32_t visionRange;
	const uint32_t moveSpeed;
	const uint32_t mass;
	bool operator==(const BaseAnimalType& x){ return &x == this; }
};
template<class Animal>
class AnimalGrowthEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalGrowthEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute()
	{
		m_animal.m_percentGrown = 100;
	}
	~AnimalGrowthEvent() { m_animal.m_growthEvent = nullptr; }
};
template<class Animal>
class AnimalFoodEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalFoodEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute() { m_animal.setNeedsFood(); }
	~AnimalFoodEvent() { m_animal.m_foodEvent = nullptr; }
};
template<class Animal>
class AnimalThirstEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalThirstEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute() { m_animal.setMaybeNeedsFluid(); }
	~AnimalThirstEvent() { m_animal.m_fluidEvent = nullptr; }
};
template<class Animal>
class AnimalTemperatureEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalTemperatureEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute() { m_animal.die(); }
	~AnimalTemperatureEvent() { m_animal.m_temperatureEvent = nullptr; }
};
template<class Animal, class Actor>
class AnimalHuntEvent : public ScheduledEvent
{
	Animal& m_animal;
	Actor* m_target;
public:
	AnimalHuntEvent(uint32_t step, Animal& a, Actor& t) : ScheduledEvent(step), m_animal(a), m_target(&t) {}
	void execute()
	{ 
		if(!m_animal.canEat(*m_target))
			m_animal.createFoodRequest();
		if(m_animal.m_destination != m_target->m_location)
			m_animal.setDestination(m_target->m_location);
	}
	~AnimalHuntEvent() { m_animal.m_temperatureEvent = nullptr; }
};
template<class DerivedAnimal, class AnimalType, class Block, class Actor, class Plant>
class BaseAnimal : public Actor
{
public:
	const AnimalType& m_animalType;
	ScheduledEventWithPercent* m_thirstEvent;
	ScheduledEventWithPercent* m_hungerEvent;
	ScheduledEventWithPercent* m_temperatureEvent;
	ScheduledEventWithPercent* m_growthEvent;
	bool m_hasFluid;
	bool m_hasFood;
	uint32_t m_percentGrown;
	bool m_alive;
	
	BaseAnimal(std::string n, const AnimalType& at, Block& l) : Actor(l, at->shape, at->moveType), m_animalType(at), m_thirstEvent(nullptr), m_hungerEvent(nullptr), m_temperatureEvent(nullptr), m_growthEvent(nullptr), m_hasFluid(true), m_hasFood(true), m_alive(true)
	{
		makeThirstEvent();
		makeHungerEvent();
		applyTemperature(m_location.getAmbientTemperature() + m_location.m_deltaTemperature);
		setMassForGrowthPercent();
	}
	DerivedAnimal derived(){ return static_cast<DerivedAnimal>(*this); }
	void makeThirstEvent(uint32_t delay)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<AnimalThirstEvent>(delay, derived());
		m_thirstEvent = event.get();
		m_location->m_area->m_eventSchedule.schedule(event);
	}
	void makeHungerEvent(uint32_t delay)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<AnimalHungerEvent>(delay, derived());
		m_hungerEvent = event.get();
		m_location->m_area->m_eventSchedule.schedule(event);
	}
	void applyTemperature(uint32_t temperature)
	{
		if(temperature >= m_animalType.minimumTemperature && temperature <= m_animalType.maximumTemperature)
		{
			if(m_temperatureEvent != nullptr)
				m_temperatureEvent.cancel();
		}
		else
		{
			if(m_temperatureEvent == nullptr)
			{
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<AnimalTemperatureEvent>(m_animalType.stepsTillDieTemperature, derived());
				m_temperatureEvent = event.get();
				m_location->m_area->m_eventSchedule.schedule(event);
			}
		}
	}
	void drink()
	{
		m_thirstEvent->cancel();
		m_hasFluid = true;
		makeThirstEvent(m_animalType.stepsNeedsFluidFrequency);
	}
	void setNeedsFluid()
	{
		if(m_thirstEvent != nullptr)
			die();
		else
		{
			makeThirstEvent(m_animalType.stepsTillDieWithoutFluid);
			m_growthEvent.cancel();
			m_location->m_area->registerDrinkRequest(derived());
		}
	}
	void eat()
	{
		m_hungerEvent->cancel();
		m_hasFood = true;
		makeHungerEvent(m_animalType.stepsNeedsFoodFrequency);
	}
	void setNeedsFood()
	{
		if(m_hungerEvent != nullptr)
			die();
		else
		{
			makeHungerEvent(m_animalType.stepsTillDieWithoutFood);
			m_growthEvent.cancel();
			createFoodRequest();
		}
	}
	void createFoodRequest()
	{
		if(m_animalType.carnivore && m_animalType.herbavore)
			m_location->m_area->registerOmnivoreRequest(derived());
		else if(m_animalType.carnivore)
			m_location->m_area->registerHuntRequest(derived());
		else if(m_animalType.herbavore)
			m_location->m_area->registerGrazeRequest(derived());
		else
			assert(false);
	}
	void updateGrowingStatus()
	{
		if(m_percentGrown != 100 && m_hasFood && m_hasFluid && m_temperatureEvent == nullptr)
		{
			if(m_growthEvent == nullptr)
			{
				uint32_t step = s_step + (m_percentGrown == 0 ?
						m_animalType.stepsTillFullyGrown :
						((m_animalType.stepsTillFullyGrown * m_percentGrown) / 100u));
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<AnimalGrowthEvent>(step, derived());
				m_growthEvent = event.get();
				m_location->m_area->m_eventSchedule.schedule(event);
			}
		}
		else if(m_growthEvent != nullptr)
		{
			m_percentGrown = growthPercent();
			m_growthEvent.cancel();
		}
	}
	void onNothingToDrink()
	{
		makeLeaveAreaRequest();
	}
	void onNothingToEath()
	{
		makeLeaveAreaRequest();
	}
	void makeLeaveAreaRequest()
	{
		m_location->m_area->registerLeaveAreaRequest(derived());
	}
	void setMassForGrowthPercent()
	{
		m_mass = (m_animalType.mass * growthPercent()) / 100u;
	}
	void die()
	{
		m_location->m_area->unregisterActor(*this);
		m_alive = false;
		onDeath();
	}
	uint32_t growthPercent() const
	{
		uint32_t output = m_percentGrown;
		if(m_growthEvent != nullptr)
			output += (m_growthEvent->percentComplete() * m_percentGrown) / 100u;
		return output;
	}
	uint32_t getVisionRange() const
	{
		return m_animalType.visionRange;
	}
	uint32_t getSpeed() const
	{
		return m_animalType.moveSpeed;
	}
	// User provided code.
	bool canEat(Plant& plant) const;
	bool canEat(Actor& actor) const;
	// Compare the likelyhood of injury vs. starvation.
	bool willHunt(Actor& actor) const;
	void onDeath();
};
