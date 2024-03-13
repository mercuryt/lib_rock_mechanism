#include "animalsArrive.h"
#include "../engine/area.h"
#include "animalSpecies.h"
#include "config.h"
#include "leaveArea.h"
#include "simulation.h"
#include "types.h"

float AnimalsArriveDramaticEvent::probabilty() { return 0.001; }

void AnimalsArriveDramaticEvent::execute(const Area& area)
{
	//TODO: Difficulty scaling.
	// Find entry point.
	Block* entrance = getEntranceToArea(area);
	if(entrance)
	{
		auto& random = area.m_simulation.m_random;
		// Pick a species and quantityp
		uint32_t quantity = 0;
		const AnimalSpecies* species = nullptr;
		Percent roll = random.getInRange(0, 100);
		bool large = false;
		bool medium = false;
		if(roll == 100)
			large = true;
		if(roll > 85)
			medium = true;
		bool carnivore = random.chance(0.1);
		if(carnivore)
		{
			if(large)
			{
				static std::vector<const AnimalSpecies*> largeCarnivors = getLargeCarnivors();
				species = largeCarnivors[random.getInRange(0, largeCarnivors.size() - 1)];
				quantity = 1;
			}
			else if(medium)
			{
				static std::vector<const AnimalSpecies*> mediumCarnivors = getMediumCarnivors();
				species = mediumCarnivors[random.getInRange(0, mediumCarnivors.size() - 1)];
				quantity = random.getInRange(1,5);
			}
			else 
			{
				static std::vector<const AnimalSpecies*> smallCarnivors = getSmallCarnivors();
				species = smallCarnivors[random.getInRange(0, smallCarnivors.size() - 1)];
				quantity = random.getInRange(5,15);
			}
		}
		else 
		{
			if(large)
			{
				static std::vector<const AnimalSpecies*> largeHerbivors = getLargeHerbivors();
				species = largeHerbivors[random.getInRange(0, largeHerbivors.size() - 1)];
				quantity = random.getInRange(1,2);
			}
			else if(medium)
			{
				static std::vector<const AnimalSpecies*> mediumHerbivors = getMediumHerbivors();
				species = mediumHerbivors[random.getInRange(0, mediumHerbivors.size() - 1)];
				quantity = random.getInRange(1,8);
			}
			else 
			{
				static std::vector<const AnimalSpecies*> smallHerbivors = getSmallHerbivors();
				species = smallHerbivors[random.getInRange(0, smallHerbivors.size() - 1)];
				quantity = random.getInRange(5,25);
			}
		}
		Percent percentHunger = std::max(0, random.getInRange(-500, 190));
		Percent percentThirst = std::max(0, random.getInRange(-500, 190));
		Percent percentTired = std::max(0, random.getInRange(-500, 190));
		// Spawn.
		std::vector<ActorId> ids;
		ids.reserve(quantity);
		while(quantity--)
		{
			Percent percentGrown = std::min(100, random.getInRange(15, 500));
			Block* location = findLocationForNear(species->shapeForPercentGrown(percentGrown), *entrance);
			if(location)
			{
				Actor& actor = area.m_simulation.createActor(ActorParamaters{
					.species=*species, 
					.percentGrown=percentGrown,
					.location=location,
					.percentHunger=percentHunger,
					.percentTired=percentTired,
					.percentThirst=percentThirst
				});
				actor.m_hasObjectives.getNext();
				ids.push_back(actor.m_id);
			}
		}
		// Schedule exit.
		Step duration = random.getInRange(uint32_t(5 * Config::stepsPerHour), uint32_t(5 * Config::stepsPerDay));
		m_scheduledEvent.schedule(*this, area.m_simulation, duration);
	}
}
std::vector<const AnimalSpecies*> getLargeCarnivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && species.eatsMeat && species.shapes.back().positions.size() > 8)
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> getMediumCarnivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && species.eatsMeat && species.shapes.back().positions.size() <= 8 && 
				species.shapes.back().positions.front().volume == Config::maxBlockVolume)
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> getSmallCarnivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && species.eatsMeat && species.shapes.back().positions.front().volume < Config::maxBlockVolume)
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> getLargeHerbivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && !species.eatsMeat && species.shapes.back().positions.size() > 8)
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> getMediumHerbivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && !species.eatsMeat && species.shapes.back().positions.size() <= 8 && species.shapes.back().positions.front().volume == Config::maxBlockVolume)
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> getSmallHerbivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && !species.eatsMeat && species.shapes.back().positions.front().volume < Config::maxBlockVolume)
			output.push_back(&species);
	}
	return output;
}
void AnimalsLeaveScheduledEvent::execute()
{
	for(Actor* actor : m_dramaticEvent.m_actors)
	{
		  if(actor->m_alive)
		  {
			  std::unique_ptr<Objective> objective = std::make_unique<LeaveAreaObjective>(*actor, 50);
			  actor->m_hasObjectives.replaceTasks(std::move(objective));
		  }
	}
}
