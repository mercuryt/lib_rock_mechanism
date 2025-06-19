#include "animalsArrive.h"
#include "../engine.h"
#include "../../area/area.h"
#include "../../animalSpecies.h"
#include "../../config.h"
#include "../../simulation/simulation.h"
#include "../../simulation/hasActors.h"
#include "../../types.h"
#include "../../util.h"
#include "../../plants.h"
#include "../../items/items.h"
#include "../../actors/actors.h"
#include <string>
AnimalsArriveDramaArc::AnimalsArriveDramaArc(DramaEngine& engine, Area& area) :
	DramaArc(engine, DramaArcType::AnimalsArrive, &area), m_scheduledEvent(area.m_eventSchedule)
{ scheduleArrive(); }
AnimalsArriveDramaArc::AnimalsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine) :
	DramaArc(data, deserializationMemo, dramaEngine),
	m_isActive(data["isActive"].get<bool>()),
	m_scheduledEvent(m_area->m_eventSchedule)
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
		SmallSet<BlockIndex> exclude;
		// Spawn.
		while(m_quantity-- != 0)
		{
			Percent percentGrown = Percent::create(std::min(100, random.getInRange(15, 500)));
			DistanceInBlocks maxBlockDistance = DistanceInBlocks::create(10);
			ShapeId shape = AnimalSpecies::shapeForPercentGrown(m_species, percentGrown);
			MoveTypeId moveType = AnimalSpecies::getMoveType(m_species);
			BlockIndex location = findLocationOnEdgeForNear(shape, moveType, m_entranceBlock, maxBlockDistance, exclude);
			if(location.exists())
			{
				exclude.insert(location);
				Actors& actors = m_area->getActors();
				ActorIndex actor = actors.create(ActorParamaters{
					.species=m_species,
					.percentGrown=percentGrown,
					.location=location,
					.percentHunger=m_hungerPercent,
					.percentTired=m_tiredPercent,
					.percentThirst=m_thristPercent
				});
				actors.objective_maybeDoNext(actor);
				m_actors.insert(actors.m_referenceData.getReference(actor));
			}
			else
			{
				scheduleContinue();
				return;
			}
		}
		if(m_quantity == 0)
		{
			scheduleDepart();
			m_isActive = false;
			m_entranceBlock.clear();
			m_species.clear();
			m_hungerPercent = Percent::create(0);
			m_thristPercent = Percent::create(0);
			m_tiredPercent = Percent::create(0);
		}
	}
	else
	{
		//TODO: Difficulty scaling.
		// Pick a species and quantity.
		auto [species, quantity] = getSpeciesAndQuantity();
		m_species = species;
		m_quantity = quantity;
		// Find entry point.
		ShapeId shape = AnimalSpecies::shapeForPercentGrown(m_species, Percent::create(100));
		m_entranceBlock = getEntranceToArea(shape, AnimalSpecies::getMoveType(species));
		if(m_entranceBlock.empty())
			scheduleArrive();
		else
		{
			m_hungerPercent = Percent::create(std::max(0, random.getInRange(-300, 190)));
			m_thristPercent = Percent::create(std::max(0, random.getInRange(-500, 190)));
			m_tiredPercent = Percent::create(std::max(0, random.getInRange(-500, 90)));
			m_isActive = true;
			// Anounce.
			std::string message = std::to_string(m_quantity.get()) + " " + AnimalSpecies::getName(m_species) + " spotted nearby.";
			m_engine.getSimulation().m_hasDialogues.createMessageBox(message, m_entranceBlock);
			// Reenter.
			callback();
		}
	}
}
void AnimalsArriveDramaArc::scheduleArrive()
{
	assert(!m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = Step::create(random.getInRange((5u * Config::stepsPerDay.get()), (15u * Config::stepsPerDay.get())));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void AnimalsArriveDramaArc::scheduleDepart()
{
	assert(m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = Step::create(random.getInRange((5u * Config::stepsPerHour.get()), (5 * Config::stepsPerDay.get())));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void AnimalsArriveDramaArc::scheduleContinue()
{
	assert(m_isActive);
	Step duration = Config::stepsPerSecond;
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
std::pair<AnimalSpeciesId, Quantity> AnimalsArriveDramaArc::getSpeciesAndQuantity() const
{
	// TODO: Species by biome.
	auto& random = m_area->m_simulation.m_random;
	Quantity quantity = Quantity::create(0);
	AnimalSpeciesId species;
	Percent roll = Percent::create(random.getInRange(0, 100));
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
			static std::vector<AnimalSpeciesId> largeCarnivors = getLargeCarnivors();
			species = random.getInVector(largeCarnivors);
			quantity = Quantity::create(1);
		}
		else if(medium)
		{
			static std::vector<AnimalSpeciesId> mediumCarnivors = getMediumCarnivors();
			species = random.getInVector(mediumCarnivors);
			quantity = Quantity::create(random.getInRange(1,5));
		}
		else
		{
			static std::vector<AnimalSpeciesId> smallCarnivors = getSmallCarnivors();
			species = random.getInVector(smallCarnivors);
			quantity = Quantity::create(random.getInRange(5,15));
		}
	}
	else
	{
		if(large)
		{
			static std::vector<AnimalSpeciesId> largeHerbivors = getLargeHerbivors();
			species = random.getInVector(largeHerbivors);
			quantity = Quantity::create(random.getInRange(1,2));
		}
		else if(medium)
		{
			static std::vector<AnimalSpeciesId> mediumHerbivors = getMediumHerbivors();
			species = random.getInVector(mediumHerbivors);
			quantity = Quantity::create(random.getInRange(1,8));
		}
		else
		{
			static std::vector<AnimalSpeciesId> smallHerbivors = getSmallHerbivors();
			species = random.getInVector(smallHerbivors);
			quantity = Quantity::create(random.getInRange(5,25));
		}
	}
	return {species, quantity};
}
bool AnimalsArriveDramaArc::isSmall(const ShapeId& shape)
{
	return CollisionVolume::create(Shape::getPositions(shape).front().offset.z()) < Config::maxBlockVolume / 4 && Shape::getPositions(shape).size() == 1;
}
bool AnimalsArriveDramaArc::isLarge(const ShapeId& shape)
{
	return Shape::getPositions(shape).size() > 8;
}
bool AnimalsArriveDramaArc::isMedium(const ShapeId& shape)
{
	return !isSmall(shape) && !isLarge(shape);
}
std::vector<AnimalSpeciesId> AnimalsArriveDramaArc::getLargeCarnivors()
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(!AnimalSpecies::getSentient(i) && AnimalSpecies::getEatsMeat(i) && isLarge(AnimalSpecies::getShapes(i).back()))
			output.push_back(i);
	return output;
}
std::vector<AnimalSpeciesId> AnimalsArriveDramaArc::getMediumCarnivors()
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(!AnimalSpecies::getSentient(i) && AnimalSpecies::getEatsMeat(i) && isMedium(AnimalSpecies::getShapes(i).back()))
			output.push_back(i);
	return output;
}
std::vector<AnimalSpeciesId> AnimalsArriveDramaArc::getSmallCarnivors()
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(!AnimalSpecies::getSentient(i) && AnimalSpecies::getEatsMeat(i) && isSmall(AnimalSpecies::getShapes(i).back()))
			output.push_back(i);
	return output;
}
std::vector<AnimalSpeciesId> AnimalsArriveDramaArc::getLargeHerbivors()
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(!AnimalSpecies::getSentient(i) && !AnimalSpecies::getEatsMeat(i) && isLarge(AnimalSpecies::getShapes(i).back()))
			output.push_back(i);
	return output;
}
std::vector<AnimalSpeciesId> AnimalsArriveDramaArc::getMediumHerbivors()
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(!AnimalSpecies::getSentient(i) && !AnimalSpecies::getEatsMeat(i) && isMedium(AnimalSpecies::getShapes(i).back()))
			output.push_back(i);
	return output;
}
std::vector<AnimalSpeciesId> AnimalsArriveDramaArc::getSmallHerbivors()
{
	std::vector<AnimalSpeciesId> output;
	for(AnimalSpeciesId i = AnimalSpeciesId::create(0); i < AnimalSpecies::size(); ++i)
		if(!AnimalSpecies::getSentient(i) && !AnimalSpecies::getEatsMeat(i) && isSmall(AnimalSpecies::getShapes(i).back()))
			output.push_back(i);
	return output;
}
void AnimalsArriveDramaArc::begin()
{
	m_isActive = false;
	callback();
}
