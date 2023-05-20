#pragma once
#include "eventSchedule.h"

template<class Block>
class AnimalGrowthEvent : public ScheduledEventWithPercent
{
	Animal<Block>& m_animal;
public:
	AnimalGrowthEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_animal(p) {}
	void execute()
	{
		m_animal.m_percentGrown = 100;
	}
	~AnimalGrowthEvent() { m_animal.m_growthEvent = nullptr; }
};
template<class Block>
class AnimalFoodEvent : public ScheduledEventWithPercent
{
	Animal<Block>& m_animal;
public:
	AnimalFoodEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_animal(p) {}
	void execute() { m_animal.setNeedsFood(); }
	~AnimalFoodEvent() { m_animal.m_foodEvent = nullptr; }
};
template<class Block>
class AnimalFluidEvent : public ScheduledEventWithPercent
{
	Animal<Block>& m_animal;
public:
	AnimalFluidEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_animal(p) {}
	void execute() { m_animal.setMaybeNeedsFluid(); }
	~AnimalFluidEvent() { m_animal.m_fluidEvent = nullptr; }
};
template<class Block>
class AnimalTemperatureEvent : public ScheduledEventWithPercent
{
	Animal<Block>& m_animal;
public:
	AnimalTemperatureEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_animal(p) {}
	void execute() { m_animal.die(); }
	~AnimalTemperatureEvent() { m_animal.m_temperatureEvent = nullptr; }
};
template<class Block>
class AnimalHuntEvent : public ScheduledEvent
{
	Animal<Block>& m_animal;
	Actor* m_target;
public:
	AnimalTemperatureEvent(uint32_t step, Animal<Block>& a, Actor& target) : ScheduledEvent(step), m_animal(a), m_target(&t) {}
	void execute()
	{ 
		if(!m_animal.canEat(*m_target))
			m_animal.createFoodRequest();
		if(m_animal.m_destination != m_target->m_location)
			m_animal.setDestination(m_target->m_location);
	}
	~AnimalTemperatureEvent() { m_animal.m_temperatureEvent = nullptr; }
};
template<class Block, class Actor>
class Animal : Actor
{
	const AnimalType* m_animalType;
	ScheduledEventWithPercent* m_thirstEvent;
	ScheduledEventWithPercent* m_hungerEvent;
	ScheduledEventWithPercent* m_temperatureEvent;
	ScheduledEventWithPercent* m_growthEvent;
	bool m_hasFluid;
	bool m_hasFood;
	uint32_t m_percentGrown;

	
	Animal(std::string n, const AnimalType* at, Block& l) : Actor(l, at->shape, at->moveType), m_animalType(at), m_thirstEvent(nullptr), m_hungerEvent(nullptr), m_temperatureEvent(nullptr), m_growthEvent(nullptr), m_hasFluid(true), m_hasFood(true)
	{
		makeThirstEvent();
		makeHungerEvent();
		applyTemperature(m_location.getAmbientTemperature() + m_location.m_deltaTemperature);
	}
	void makeThirstEvent(uint32_t delay)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<ThirstEvent>(delay, *this);
		m_thirstEvent = event.get();
		m_location->m_area->m_eventSchedule.schedule(event);
	}
	void makeHungerEvent(uint32_t delay)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<HungerEvent>(delay, *this);
		m_hungerEvent = event.get();
		m_location->m_area->m_eventSchedule.schedule(event);
	}
	void applyTemperature(uint32_t temperature)
	{
		if(temperature >= m_animalType->minimumTemperature && temperature <= m_animalType->maximumTemperature)
		{
			if(m_temperatureEvent != nullptr)
				m_temperatureEvent.cancel();
		}
		else
		{
			if(m_temperatureEvent == nullptr)
			{
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<AnimalTemperatureEvent>(m_animalType.m_stepsTillDieTemperature, *this);
				m_temperatureEvent = event.get();
				m_location->m_area->m_eventSchedule.schedule(event);
			}
		}
	}
	void drink()
	{
		m_thirstEvent->cancel();
		m_hasFluid = true;
		makeThirstEvent(m_animalType->stepsNeedsFluidFrequency);
	}
	void setNeedsFluid()
	{
		if(m_thirstEvent != nullptr)
			die();
		else
		{
			makeThirstEvent(m_animalType->stepsTillDieWithoutFluid);
			m_growthEvent.cancel();
			m_location->m_area->registerDrinkRequest(*this);
		}
	}
	void eat()
	{
		m_hungerEvent->cancel();
		m_hasFood = true;
		makeHungerEvent(m_animalType->stepsNeedsFoodFrequency);
	}
	void setNeedsFood()
	{
		if(m_hungerEvent != nullptr)
			die();
		else
		{
			makeHungerEvent(m_animalType->stepsTillDieWithoutFood);
			m_growthEvent.cancel();
			createFoodRequest();
		}
	}
	void createFoodRequest()
	{
		if(m_animalType->carnivore && m_animalType->herbavore)
			m_location->m_area->registerOmnivoreRequest(*this);
		else if(m_animalType->carnivore)
			m_location->m_area->registerHuntRequest(*this);
		else if(m_animalType->herbavore)
			m_location->m_area->registerGrazeRequest(*this);
		else
			assert(false);
	}
	void updateGrowingStatus()
	{
		if(m_percentGrown != 100 && m_hasFood && m_hasFluid && m_temperatureEvent == nullptr
				{
				if(m_growthEvent == nullptr)
				{
				uint32_t step = s_step + (m_percentGrown == 0 ?
						m_animalType->stepsTillFullyGrown :
						((m_animalType->stepsTillFullyGrown * m_percentGrown) / 100u));
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<AnimalGrowthEvent>(step, *this);
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
					m_location->m_area->registerLeaveAreaRequest(*this);
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
					return m_animalType->visionRange;
				}
				uint32_t getSpeed() const
				{
					return m_animalType->moveSpeed;
				}
				bool canEat(Plant<Block> plant) const
				{
					assert(m_animalType.herbavore);
					return true;
				}
				bool canEat(Actor& animal) const
				{
					assert(m_animalType.carnivore);
					return m_animalType.;
				}
				void die()
				{
					exitArea();
				}
}
