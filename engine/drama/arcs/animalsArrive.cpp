#include "animalsArrive.h"
#include "../engine.h"
#include "../../area.h"
#include "../../animalSpecies.h"
#include "../../config.h"
#include "../../leaveArea.h"
#include "../../simulation.h"
AnimalsArriveDramaArc::AnimalsArriveDramaArc(DramaEngine& engine, Area& area) : 
	DramaArc(engine, DramaArcType::AnimalsArrive, &area), m_scheduledEvent(area.m_simulation.m_eventSchedule)
{ scheduleArrive(); }
AnimalsArriveDramaArc::AnimalsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo) : 
	DramaArc(data, deserializationMemo),
	m_isActive(data["isActive"].get<bool>()),
	m_scheduledEvent(m_area->m_simulation.m_eventSchedule)
{
	m_scheduledEvent.schedule(*this, m_area->m_simulation, data["start"].get<Step>(), data["duration"].get<Step>());
}
Json AnimalsArriveDramaArc::toJson() const
{
	Json data = DramaArc::toJson();
	data["isActive"] = m_isActive;
	data["duration"] = m_scheduledEvent.duration();
	data["start"] = m_scheduledEvent.getStartStep();
	return data;
}
void AnimalsArriveDramaArc::callback()
{
	auto& random = m_area->m_simulation.m_random;
	if(m_isActive)
	{
		std::unordered_set<Block*> exclude;
		// Spawn.
		while(m_quantity--)
		{
			Percent percentGrown = std::min(100, random.getInRange(15, 500));
			constexpr DistanceInBlocks maxBlockDistance = 10;
			Block* location = findLocationOnEdgeForNear(m_species->shapeForPercentGrown(percentGrown), m_species->moveType, *m_entranceBlock, maxBlockDistance, exclude);
			if(location)
			{
				exclude.insert(location);
				Actor& actor = m_area->m_simulation.createActor(ActorParamaters{
					.species=*m_species, 
					.percentGrown=percentGrown,
					.location=location,
					.percentHunger=m_hungerPercent,
					.percentTired=m_tiredPercent,
					.percentThirst=m_thristPercent
				});
				actor.m_hasObjectives.getNext();
				m_actors.push_back(&actor);
			}
			else
			{
				scheduleContinue();
				return;
			}
		}
		if(!m_quantity)
		{
			scheduleDepart();
			m_isActive = false;
			m_entranceBlock = nullptr;
			m_species = nullptr;
			m_hungerPercent = 0;
			m_thristPercent = 0;
			m_tiredPercent = 0;
		}
	}
	else
	{
		//TODO: Difficulty scaling.
		auto [species, quantity] = getSpeciesAndQuantity();
		m_species = species;
		m_quantity = quantity;
		// Find entry point.
		m_entranceBlock = getEntranceToArea(*m_area, species->shapeForPercentGrown(100), species->moveType);
		if(!m_entranceBlock)
			scheduleArrive();
		else
		{
			// Pick a species and quantity.
			m_hungerPercent = std::max(0, random.getInRange(-500, 190));
			m_thristPercent = std::max(0, random.getInRange(-500, 190));
			m_tiredPercent = std::max(0, random.getInRange(-500, 90));
			m_isActive = true;
			// Reenter.
			callback();
		}
	}
}
void AnimalsArriveDramaArc::scheduleArrive()
{
	assert(!m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = random.getInRange(uint32_t(5 * Config::stepsPerDay), uint32_t(15 * Config::stepsPerDay));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void AnimalsArriveDramaArc::scheduleDepart()
{
	assert(m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = random.getInRange(uint32_t(5 * Config::stepsPerHour), uint32_t(5 * Config::stepsPerDay));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void AnimalsArriveDramaArc::scheduleContinue()
{
	assert(m_isActive);
	Step duration = Config::stepsPerSecond;
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
std::pair<const AnimalSpecies*, uint32_t> AnimalsArriveDramaArc::getSpeciesAndQuantity() const
{
	// TODO: Species by biome.
	auto& random = m_area->m_simulation.m_random;
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
	return {species, quantity};
}
bool AnimalsArriveDramaArc::isSmall(const Shape& shape)
{
	return Volume(shape.positions.front()[3]) < Config::maxBlockVolume / 4 && shape.positions.size() == 1;
}
bool AnimalsArriveDramaArc::isLarge(const Shape& shape)
{
	return shape.positions.size() > 8;
}
bool AnimalsArriveDramaArc::isMedium(const Shape& shape)
{
	return !isSmall(shape) && !isLarge(shape);
}
std::vector<const AnimalSpecies*> AnimalsArriveDramaArc::getLargeCarnivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && species.eatsMeat && isLarge(*species.shapes.back()))
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> AnimalsArriveDramaArc::getMediumCarnivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && species.eatsMeat && isMedium(*species.shapes.back()))
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> AnimalsArriveDramaArc::getSmallCarnivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && species.eatsMeat && isSmall(*species.shapes.back()))
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> AnimalsArriveDramaArc::getLargeHerbivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && !species.eatsMeat && isLarge(*species.shapes.back()))
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> AnimalsArriveDramaArc::getMediumHerbivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && !species.eatsMeat && isMedium(*species.shapes.back()))
			output.push_back(&species);
	}
	return output;
}
std::vector<const AnimalSpecies*> AnimalsArriveDramaArc::getSmallHerbivors()
{
	std::vector<const AnimalSpecies*> output;
	for(const AnimalSpecies& species : AnimalSpecies::data)
	{
		if(!species.sentient && !species.eatsMeat && isSmall(*species.shapes.back()))
			output.push_back(&species);
	}
	return output;
}
void AnimalsArriveDramaArc::begin()
{
	m_isActive = false;
	callback();
}
