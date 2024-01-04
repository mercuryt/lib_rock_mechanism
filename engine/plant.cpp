#include "plant.h"
#include "construct.h"
#include "deserializationMemo.h"
#include "eventSchedule.h"
#include "types.h"
#include "util.h"
#include "plant.h"
#include "block.h"
#include "area.h"

#include <algorithm>
Plant::Plant(Block& l, const PlantSpecies& pt, const Shape* shape, Percent pg, Volume volumeFluidRequested, Step needsFluidEventStart, bool temperatureIsUnsafe, Step unsafeTemperatureEventStart, uint32_t harvestableQuantity, Percent percentFoliage) :
       	HasShape(l.m_area->m_simulation, (shape ? *shape : pt.shapeForPercentGrown(pg)), true), m_location(l), m_fluidSource(nullptr), m_plantSpecies(pt), m_growthEvent(l.m_area->m_simulation.m_eventSchedule), m_fluidEvent(l.m_area->m_simulation.m_eventSchedule), m_temperatureEvent(l.m_area->m_simulation.m_eventSchedule), m_endOfHarvestEvent(l.m_area->m_simulation.m_eventSchedule), m_foliageGrowthEvent(l.m_area->m_simulation.m_eventSchedule), m_percentGrown(pg), m_quantityToHarvest(harvestableQuantity), m_percentFoliage(percentFoliage), m_reservable(1), m_volumeFluidRequested(volumeFluidRequested), m_wildGrowth(0)
{
	assert(m_location.m_hasPlant.canGrowHereAtSomePointToday(m_plantSpecies));
	m_fluidEvent.schedule(m_plantSpecies.stepsNeedsFluidFrequency, *this, needsFluidEventStart);
	if(temperatureIsUnsafe)
		m_temperatureEvent.schedule(m_plantSpecies.stepsTillDieFromTemperature, *this, unsafeTemperatureEventStart);
	updateGrowingStatus();
	m_location.m_area->m_hasFarmFields.removeAllSowSeedsDesignations(m_location);
	m_location.m_hasShapes.enter(*this);
	uint8_t wildGrowth = pt.wildGrowthForPercentGrown(m_percentGrown);
	if(wildGrowth)
		doWildGrowth(wildGrowth);
}
Plant::Plant(const Json& data, DeserializationMemo& deserializationMemo, Block& l) :
	HasShape(data, deserializationMemo), m_location(l),
	m_fluidSource(&deserializationMemo.blockReference(data["fluidSource"])), m_plantSpecies(*data["species"].get<const PlantSpecies*>()),
	m_growthEvent(l.m_area->m_simulation.m_eventSchedule), m_fluidEvent(l.m_area->m_simulation.m_eventSchedule), 
	m_temperatureEvent(l.m_area->m_simulation.m_eventSchedule), m_endOfHarvestEvent(l.m_area->m_simulation.m_eventSchedule), 
	m_foliageGrowthEvent(l.m_area->m_simulation.m_eventSchedule), m_percentGrown(data["precentGrown"].get<Percent>()), 
	m_quantityToHarvest(data["harvestableQuantity"].get<uint32_t>()), m_percentFoliage(data["percentFoliage"].get<Percent>()), 
	m_reservable(1), m_volumeFluidRequested(data["volumeFluidRequested"].get<Volume>())
{
	if(data.contains("growthEventStart"))
		m_growthEvent.schedule(data["growthEventDuration"].get<Step>(), *this, data["growthEventStart"].get<Step>());
	if(data.contains("fluidEventStart"))
		m_fluidEvent.schedule(data["fluidEventDuration"].get<Step>(), *this, data["fluidEventStart"].get<Step>());
	if(data.contains("temperatureEventStart"))
		m_temperatureEvent.schedule(data["temperatureEventDuration"].get<Step>(), *this, data["temperatureEventStart"].get<Step>());
	if(data.contains("endOfHarvestEventStart"))
		m_endOfHarvestEvent.schedule(data["endOfHarvestEventDuration"].get<Step>(), *this, data["endOfHarvestEventStart"].get<Step>());
	if(data.contains("foliageGrowthEventStart"))
		m_foliageGrowthEvent.schedule(data["foliageGrowthEventDuration"].get<Step>(), *this, data["foliageGrowthEventStart"].get<Step>());
	m_location.m_hasShapes.enter(*this);
}
Json Plant::toJson() const
{
	Json data = HasShape::toJson();
	data["fluidSource"] = m_fluidSource;
	data["species"] = m_plantSpecies;
	data["percentGrown"] = getGrowthPercent();
	data["harvestableQuantity"] = m_quantityToHarvest;
	data["percentFoliage"] = m_percentFoliage;
	data["volumeFluidRequested"] = m_volumeFluidRequested;
	if(m_growthEvent.exists())
	{
		data["growthEventStart"] = m_growthEvent.getStartStep();
		data["growthEventDuration"] = m_growthEvent.duration();
	}
	if(m_fluidEvent.exists())
	{
		data["fluidEventStart"] = m_fluidEvent.getStartStep();
		data["fluidEventDuration"] = m_fluidEvent.duration();
	}
	if(m_temperatureEvent.exists())
	{
		data["temperatureEventStart"] = m_temperatureEvent.getStartStep();
		data["temperatureEventDuration"] = m_temperatureEvent.duration();
	}
	if(m_endOfHarvestEvent.exists())
	{
		data["endOfHarvestEventStart"] = m_endOfHarvestEvent.getStartStep();
		data["endOfHarvestEventDuration"] = m_endOfHarvestEvent.duration();
	}
	if(m_foliageGrowthEvent.exists())
	{
		data["foliageGrowthEventStart"] = m_foliageGrowthEvent.getStartStep();
		data["foliageGrowthEventDuration"] = m_foliageGrowthEvent.duration();
	}
	return data;
}
void to_json(Json& data, const Plant* const& plant){ data = plant->m_location.positionToJson(); }
void Plant::die()
{
	m_growthEvent.maybeUnschedule();
	m_fluidEvent.maybeUnschedule();
	m_temperatureEvent.maybeUnschedule();
	m_endOfHarvestEvent.maybeUnschedule();
	m_foliageGrowthEvent.maybeUnschedule();
	m_location.m_isPartOfFarmField.removeAllHarvestDesignations();
	m_location.m_isPartOfFarmField.removeAllGiveFluidDesignations();
	Block& location = m_location;
	m_location.m_area->m_hasPlants.erase(*this);
	location.m_isPartOfFarmField.maybeDesignateForSowingIfPartOfFarmField();
	m_location.m_hasShapes.exit(*this);
	//TODO: Create rot away event.
}
void Plant::setTemperature(Temperature temperature)
{
	if(temperature >= m_plantSpecies.minimumGrowingTemperature && temperature <= m_plantSpecies.maximumGrowingTemperature)
	{
		if(m_temperatureEvent.exists())
			m_temperatureEvent.unschedule();
	}
	else
		if(!m_temperatureEvent.exists())
			m_temperatureEvent.schedule(m_plantSpecies.stepsTillDieFromTemperature, *this);
	updateGrowingStatus();
}
void Plant::setHasFluidForNow()
{
	m_volumeFluidRequested = 0;
	if(m_fluidEvent.exists())
		m_fluidEvent.unschedule();
	m_fluidEvent.schedule(m_plantSpecies.stepsNeedsFluidFrequency, *this);
	updateGrowingStatus();
	m_location.m_isPartOfFarmField.removeAllGiveFluidDesignations();
}
void Plant::setMaybeNeedsFluid()
{
	Step stepsTillNextFluidEvent;
	if(hasFluidSource())
	{
		m_volumeFluidRequested = 0;
		stepsTillNextFluidEvent = m_plantSpecies.stepsNeedsFluidFrequency;
	}
	else if(m_volumeFluidRequested != 0)
	{
		die();
		return;
	}
	else // Needs fluid, stop growing and set death timer.
	{
		m_volumeFluidRequested = std::max(1, util::scaleByPercent(m_plantSpecies.volumeFluidConsumed, getGrowthPercent()));
		stepsTillNextFluidEvent = m_plantSpecies.stepsTillDieWithoutFluid;
		m_location.m_isPartOfFarmField.designateForGiveFluidIfPartOfFarmField(*this);
	}
	m_fluidEvent.maybeUnschedule();
	m_fluidEvent.schedule(stepsTillNextFluidEvent, *this);
	updateGrowingStatus();
}
void Plant::addFluid(Volume volume, const FluidType& fluidType)
{
	assert(volume != 0);
	assert(fluidType == m_plantSpecies.fluidType);
	assert(volume <= m_volumeFluidRequested);
	m_volumeFluidRequested -= volume;
	if(m_volumeFluidRequested == 0)
		setHasFluidForNow();
}
bool Plant::hasFluidSource()
{
	if(m_fluidSource != nullptr && m_fluidSource->m_fluids.contains(&m_plantSpecies.fluidType))
		return true;
	m_fluidSource = nullptr;
	for(Block* block : m_location.collectAdjacentsInRange(getRootRange()))
		if(block->m_fluids.contains(&m_plantSpecies.fluidType))
		{
			m_fluidSource = block;
			return true;
		}
	return false;
}
void Plant::setDayOfYear(uint32_t dayOfYear)
{
	if(m_plantSpecies.harvestData != nullptr && dayOfYear == m_plantSpecies.harvestData->dayOfYearToStart)
		setQuantityToHarvest();
}
void Plant::setQuantityToHarvest()
{
	Step delay = m_plantSpecies.harvestData->stepsDuration;
	m_endOfHarvestEvent.schedule(delay, *this);
	m_quantityToHarvest = util::scaleByPercent(m_plantSpecies.harvestData->itemQuantity, getGrowthPercent());
	m_location.m_isPartOfFarmField.designateForHarvestIfPartOfFarmField(*this);
}
void Plant::harvest(uint32_t quantity)
{
	assert(quantity >= m_quantityToHarvest);
	m_quantityToHarvest -= quantity;
	if(m_quantityToHarvest == 0)
		endOfHarvest();
}
void Plant::endOfHarvest()
{
	m_location.m_isPartOfFarmField.removeAllHarvestDesignations();
	if(m_plantSpecies.annual)
		die();
	else
		m_quantityToHarvest = 0;
}
void Plant::updateGrowingStatus()
{
	if(m_percentGrown == 100)
		return;
	if(m_volumeFluidRequested == 0 && m_location.m_exposedToSky == m_plantSpecies.growsInSunLight && !m_temperatureEvent.exists() && getPercentFoliage() >= Config::minimumPercentFoliageForGrow)
	{
		if(!m_growthEvent.exists())
		{
			Step delay = (m_percentGrown == 0 ?
					m_plantSpecies.stepsTillFullyGrown :
					util::scaleByPercent(m_plantSpecies.stepsTillFullyGrown, m_percentGrown));
			m_growthEvent.schedule(delay, *this);
		}
	}
	else
	{
		if(m_growthEvent.exists())
		{
			m_percentGrown = getGrowthPercent();
			m_growthEvent.unschedule();
		}
	}
}
Percent Plant::getGrowthPercent() const
{
	Percent output = m_percentGrown;
	if(m_growthEvent.exists())
	{
		if(m_percentGrown != 0)
			output += (m_growthEvent.percentComplete() * (100u - m_percentGrown)) / 100u;
		else
			output = m_growthEvent.percentComplete();
	}
	return output;
}
uint32_t Plant::getRootRange() const
{
	return m_plantSpecies.rootRangeMin + ((m_plantSpecies.rootRangeMax - m_plantSpecies.rootRangeMin) * getGrowthPercent()) / 100;
}
Percent Plant::getPercentFoliage() const
{
	Percent output = m_percentFoliage;
	if(m_foliageGrowthEvent.exists())
	{
		if(m_percentFoliage != 0)
			output += (m_foliageGrowthEvent.percentComplete() * (100u - m_percentFoliage)) / 100u;
		else
			output = m_foliageGrowthEvent.percentComplete();
	}
	return output;
}
Mass Plant::getFoliageMass() const
{
	Mass maxFoliageForType = util::scaleByPercent(m_plantSpecies.adultMass, Config::percentOfPlantMassWhichIsFoliage);
	Mass maxForGrowth = util::scaleByPercent(maxFoliageForType, getGrowthPercent());
	return util::scaleByPercent(maxForGrowth, getPercentFoliage());
}
Volume Plant::getFluidDrinkVolume() const
{
	return util::scaleByPercent(m_plantSpecies.volumeFluidConsumed, Config::percentOfPlantMassWhichIsFoliage);
}
void Plant::removeFoliageMass(Mass mass)
{
	Mass maxFoliageForType = util::scaleByPercent(m_plantSpecies.adultMass, Config::percentOfPlantMassWhichIsFoliage);
	Mass maxForGrowth = util::scaleByPercent(maxFoliageForType, getGrowthPercent());
	Percent percentRemoved = std::max(1u, ((maxForGrowth - mass) / maxForGrowth) * 100u);
	m_percentFoliage = getPercentFoliage();
	assert(m_percentFoliage >= percentRemoved);
	m_percentFoliage -= percentRemoved;
	m_foliageGrowthEvent.maybeUnschedule();
	makeFoliageGrowthEvent();
	updateGrowingStatus();
}
void Plant::doWildGrowth(uint8_t count)
{
	m_wildGrowth += count;
	assert(m_wildGrowth <= m_plantSpecies.maxWildGrowth);
	Simulation& simulation = m_location.m_area->m_simulation;
	while(count)
	{
		count--;
		std::vector<Block*> candidates;
		for(Block* block : getAdjacentBlocks())
			if(block->m_hasShapes.anythingCanEnterEver() && block->m_hasShapes.getTotalVolume() == 0 )
				candidates.push_back(block);
		std::vector<Block*> vector(candidates.begin(), candidates.end());
		Block& toGrowInto = *simulation.m_random.getInVector(vector);
		std::array<int32_t, 3> offset = m_location.relativeOffsetTo(toGrowInto);
		// Use the volume of the location position as the volume of the new growth position.
		std::array<int32_t, 4> position = {offset[0], offset[1], offset[2], m_shape->positions[0][4]};
		m_shape = &simulation.m_shapes.mutateAdd(*m_shape, position);
	}
}
void Plant::removeFruitQuantity(uint32_t quantity)
{
	assert(quantity <= m_quantityToHarvest);
	m_quantityToHarvest -= quantity;
}
Mass Plant::getFruitMass() const
{
	static const MaterialType& fruitType = MaterialType::byName("fruit");
	return m_plantSpecies.harvestData->fruitItemType.volume * fruitType.density * m_quantityToHarvest;
}
void Plant::makeFoliageGrowthEvent()
{
	assert(m_percentFoliage != 100);
	Step delay = util::scaleByInversePercent(m_plantSpecies.stepsTillFoliageGrowsFromZero, m_percentFoliage);
	m_foliageGrowthEvent.schedule(delay, *this);
}
void Plant::foliageGrowth()
{
	m_percentFoliage = 100;
	updateGrowingStatus();
}
Step Plant::getStepAtWhichPlantWillDieFromLackOfFluid() const
{
	assert(m_volumeFluidRequested != 0);
	return m_fluidEvent.getStep();
}
// Events.
PlantGrowthEvent::PlantGrowthEvent(const Step delay, Plant& p, Step start) : 
	ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay, start), m_plant(p) {}
PlantFoliageGrowthEvent::PlantFoliageGrowthEvent(const Step delay, Plant& p, Step start) : 
	ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay, start), m_plant(p) {}
PlantEndOfHarvestEvent::PlantEndOfHarvestEvent(const Step delay, Plant& p, Step start) :
	ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay, start), m_plant(p) {}
PlantFluidEvent::PlantFluidEvent(const Step delay, Plant& p, Step start) :
	ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay, start), m_plant(p) {}
PlantTemperatureEvent::PlantTemperatureEvent(const Step delay, Plant& p, Step start) :
	ScheduledEventWithPercent(p.m_location.m_area->m_simulation, delay, start), m_plant(p) {}
// HasPlant.
void HasPlant::addPlant(const PlantSpecies& plantSpecies, Percent growthPercent)
{
	assert(m_plant == nullptr);
	m_plant = &m_block.m_area->m_hasPlants.emplace(m_block, plantSpecies, growthPercent);
}
void HasPlant::updateGrowingStatus()
{
	if(m_plant != nullptr)
		m_plant->updateGrowingStatus();
}
void HasPlant::clearPointer()
{
	assert(m_plant != nullptr);
	m_plant = nullptr;
}
void HasPlant::setTemperature(Temperature temperature)
{
	if(m_plant != nullptr)
		m_plant->setTemperature(temperature);
}
void HasPlant::erase()
{
	assert(m_plant != nullptr);
	m_plant = nullptr;
}
bool HasPlant::canGrowHereCurrently(const PlantSpecies& plantSpecies) const
{
	Temperature temperature = m_block.m_blockHasTemperature.get();
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(plantSpecies.growsInSunLight != m_block.m_outdoors)
		return false;
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	if(m_block.m_adjacents[0] == nullptr || !m_block.m_adjacents[0]->isSolid() || m_block.m_adjacents[0]->getSolidMaterial() != dirtType)
		return false;
	return true;
}
bool HasPlant::canGrowHereAtSomePointToday(const PlantSpecies& plantSpecies) const
{
	Temperature temperature = m_block.m_blockHasTemperature.getDailyAverageAmbientTemperature();
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(plantSpecies.growsInSunLight != m_block.m_outdoors)
		return false;
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	if(m_block.m_adjacents[0] == nullptr || !m_block.m_adjacents[0]->isSolid() || m_block.m_adjacents[0]->getSolidMaterial() != dirtType)
		return false;
	return true;
}
Plant& HasPlants::emplace(Block& location, const PlantSpecies& species, Percent percentGrowth, Volume volumeFluidRequested, Step needsFluidEventStart, bool temperatureIsUnsafe, Step unsafeTemperatureEventStart, uint32_t harvestableQuantity, Percent percentFoliage)
{
	assert(location.m_hasPlant.canGrowHereAtSomePointToday(species));
	assert(!location.m_hasPlant.exists());
	Plant& plant = m_plants.emplace_back(location, species, percentGrowth, volumeFluidRequested, needsFluidEventStart, temperatureIsUnsafe, unsafeTemperatureEventStart, harvestableQuantity, percentFoliage);
	// TODO: plants above ground but under roof?
	if(!location.m_underground)
		m_plantsOnSurface.insert(&plant);
	return plant;
}
Plant& HasPlants::emplace(const Json& data, DeserializationMemo& deserializationMemo)
{
	return emplace(
		deserializationMemo.blockReference(data["location"]),
		*data["species"].get<const PlantSpecies*>(),
		data["percentGrowth"].get<Percent>(),
		data["volumeFluidRequested"].get<Volume>(),
		data["needsFluidEventStart"].get<Step>(),
		data["temperatureIsUnsafe"].get<bool>(),
		data["unsafeTemperatureEventStart"].get<Step>(),
		data["harvestableQuantity"].get<uint32_t>(),
		data["percentFoliage"].get<Percent>()
	);
}
void HasPlants::erase(Plant& plant)
{
	assert(plant.m_location.m_hasPlant.get() == plant);
	plant.m_location.m_hasPlant.erase();
	//TODO: store iterator in plant.
	auto found = std::ranges::find(m_plants, plant);
	assert(found != m_plants.end());
	m_plants.erase(found);
	m_plantsOnSurface.erase(&plant);
}
void HasPlants::onChangeAmbiantSurfaceTemperature()
{
	for(Plant* plant : m_plantsOnSurface)
		plant->setTemperature(plant->m_location.m_blockHasTemperature.get());
}
