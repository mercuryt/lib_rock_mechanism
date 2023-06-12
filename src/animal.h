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
	const uint32_t birthMass;
	const uint32_t adultMass;
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
	~AnimalGrowthEvent() { m_animal.m_growthEvent.clearPointer(); }
};
template<class Animal>
class AnimalHungerEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalHungerEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute() { m_animal.setNeedsFood(); }
	~AnimalHungerEvent() { m_animal.m_foodEvent.clearPointer(); }
};
template<class Animal>
class AnimalThirstEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalThirstEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute() { m_animal.setNeedsFluid(); }
	~AnimalThirstEvent() { m_animal.m_fluidEvent.clearPointer(); }
};
template<class Animal>
class AnimalTemperatureEvent : public ScheduledEventWithPercent
{
	Animal& m_animal;
public:
	AnimalTemperatureEvent(uint32_t step, Animal a) : ScheduledEventWithPercent(step), m_animal(a) {}
	void execute() { m_animal.die(); }
	~AnimalTemperatureEvent() { m_animal.m_temperatureEvent.clearPointer(); }
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
	~AnimalHuntEvent() { m_animal.m_temperatureEvent.clearPointer(); }
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
	uint32_t m_percentGrown;
	uint32_t m_massFoodRequested;
	bool m_alive;

	BaseAnimal(std::string n, const AnimalType& at, Block& l, uint32_t pg) : Actor(l, at->shape, at->moveType, at->moveSpeed, 0), m_animalType(at), m_hasFluid(true), m_percentGrown(pg)
	{
		m_thirstEvent->schedule(s_step + m_animalType.stepsNeedsFluidFrequency, derived());
		m_hungerEvent->schedule(s_step + m_animalType.stepsNeedsFoodFrequency, derived());
		applyTemperature(getLocation().getAmbientTemperature() + getLocation().m_deltaTemperature);
		setMassForGrowthPercent();
	}
	DerivedAnimal& derived(){ return static_cast<DerivedAnimal>(*this); }
	void applyTemperature(uint32_t temperature)
	{
		if(temperature >= m_animalType.minimumTemperature && temperature <= m_animalType.maximumTemperature)
		{
			if(!m_temperatureEvent.empty())
				m_temperatureEvent->unschedule();
		}
		else
		{
			if(m_temperatureEvent.empty())
			{
				m_temperatureEvent.schedule(::s_step + m_animalType.stepsTillDieTemperature, derived());
				updateGrowingStatus();
			}
		}
	}
	void drink()
	{
		m_thirstEvent.unschedule();
		m_hasFluid = true;
		m_thirstEvent.schedule(::s_step + m_animalType.stepsNeedsFluidFrequency, derived());
	}
	void setNeedsFluid()
	{
		if(!m_thirstEvent.empty())
			die();
		else
		{
			m_thirstEvent.schedule(::s_step + m_animalType.stepsTillDieWithoutFluid, derived());
			m_growthEvent.unschedule();
			m_hasObjectives.addNeed(std::make_unique<DrinkObjective>(*this));
		}
	}
	void eat(uint32_t mass)
	{
		assert(mass <= m_massFoodRequested);
		m_massFoodRequested -= mass;
		m_hungerEvent.unschedule();
		if(m_massFoodRequested == 0)
			updateGrowingStatus();
		else
		{
			uint32_t delay = util::scaleByInverseFraction(m_animalType.stepsTillDieWithoutFood, m_massFoodRequested, massFoodForBodyMass());
			makeHungerEvent(delay);
			m_hasObjectives.addNeed(std::make_unique<EatObjective>(*this));
		}
	}
	void setNeedsFood()
	{
		if(!m_hungerEvent.empty())
			die();
		else
		{
			makeHungerEvent(m_animalType.stepsTillDieWithoutFood);
			m_growthEvent->unschedule();
			m_massFoodRequested = massFoodForBodyMass();
			createFoodRequest();
		}
	}
	uint32_t massFoodForBodyMass() const
	{
		return getMass() / Config::unitsOfBodyMassPerUnitOfFoodRequestedMass;
	}
	uint32_t getMassFoodRequested() const
	{
		assert(m_percentHunger != 0 && !m_hungerEvent.empty());
		return util::scaleByPercent(massFoodForBodyMass(), 100u - m_hungerEvent->percentComplete());
	}
	void updateGrowingStatus()
	{
		if(m_percentGrown != 100 && m_massFoodRequested == 0 && m_hasFluid && m_temperatureEvent.empty())
		{
			if(m_growthEvent.empty())
			{
				uint32_t step = s_step + (m_percentGrown == 0 ?
						m_animalType.stepsTillFullyGrown :
						((m_animalType.stepsTillFullyGrown * m_percentGrown) / 100u));
				m_growthEvent.schedule(step, derived());
			}
		}
		else if(!m_growthEvent.empty())
		{
			m_percentGrown = growthPercent();
			m_growthEvent->unschedule();
		}
	}
	void onNothingToDrink()
	{
		makeLeaveAreaRequest();
	}
	void onNothingToEat()
	{
		makeLeaveAreaRequest();
	}
	void makeLeaveAreaRequest()
	{
		getLocation()->m_area->registerLeaveAreaRequest(derived());
	}
	void setMassForGrowthPercent()
	{
		m_mass = m_animalType.birthMass + ((m_animalType.adultMass - m_animalType.birthMass) * growthPercent()) / 100u;
	}
	void die()
	{
		getLocation()->m_area->unregisterActor(*this);
		m_alive = false;
		onDeath();
	}
	uint32_t growthPercent() const
	{
		uint32_t output = m_percentGrown;
		if(!m_growthEvent.empty())
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
