#include "plant.h"
#include "config.h"
#include "construct.h"
#include "deserializationMemo.h"
#include "eventSchedule.h"
#include "types.h"
#include "util.h"
#include "plant.h"
#include "item.h"
#include "area.h"
#include "simulation.h"
#include "fluidType.h"
#include "moveType.h"

#include <algorithm>
#include <ranges>
const std::pair<const Shape*, uint8_t> PlantSpecies::shapeAndWildGrowthForPercentGrown(Percent percentGrown) const
{
	uint8_t totalGrowthStages = shapes.size() + maxWildGrowth;
	uint8_t growthStage = util::scaleByPercentRange(1, totalGrowthStages, percentGrown);
	size_t index = std::min(static_cast<size_t>(growthStage), shapes.size()) - 1;
	uint8_t wildGrowthSteps = shapes.size() < growthStage ? growthStage - shapes.size() : 0u;
	return std::make_pair(shapes.at(index), wildGrowthSteps);
}
// Static method.
const PlantSpecies& PlantSpecies::byName(std::string name)
{
	auto found = std::ranges::find(plantSpeciesDataStore, name, &PlantSpecies::name);
	assert(found != plantSpeciesDataStore.end());
	return *found;
}
Plant::Plant(Area& area, BlockIndex location, const PlantSpecies& pt, const Shape* shape, Percent pg, Volume volumeFluidRequested, Step needsFluidEventStart, bool temperatureIsUnsafe, Step unsafeTemperatureEventStart, uint32_t harvestableQuantity, Percent percentFoliage) :
	HasShape(area.m_simulation, (shape ? *shape : pt.shapeForPercentGrown(pg)), true), 
	m_reservable(1), 
	m_growthEvent(area.m_simulation.m_eventSchedule), 
	m_shapeGrowthEvent(area.m_simulation.m_eventSchedule), 
	m_fluidEvent(area.m_simulation.m_eventSchedule), 
	m_temperatureEvent(area.m_simulation.m_eventSchedule), 
	m_endOfHarvestEvent(area.m_simulation.m_eventSchedule), 
	m_foliageGrowthEvent(area.m_simulation.m_eventSchedule), 
	m_plantSpecies(pt), 
	m_quantityToHarvest(harvestableQuantity), 
	m_percentGrown(pg), 
	m_percentFoliage(percentFoliage), 
	m_volumeFluidRequested(volumeFluidRequested), 
	m_wildGrowth(0)
{
	setLocation(location, &area);
	auto& blocks = area.getBlocks();
	assert(blocks.plant_canGrowHereEver(location, m_plantSpecies));
	uint8_t wildGrowth = pt.wildGrowthForPercentGrown(m_percentGrown);
	if(wildGrowth)
		doWildGrowth(wildGrowth);
	m_fluidEvent.schedule(m_plantSpecies.stepsNeedsFluidFrequency, *this, needsFluidEventStart);
	if(temperatureIsUnsafe)
		m_temperatureEvent.schedule(m_plantSpecies.stepsTillDieFromTemperature, *this, unsafeTemperatureEventStart);
	updateGrowingStatus();
	area.m_hasFarmFields.removeAllSowSeedsDesignations(m_location);
	// Fruit.
	if(m_plantSpecies.harvestData)
	{
		uint16_t day = area.m_simulation.getDateTime().day;
		uint16_t start = m_plantSpecies.harvestData->dayOfYearToStart;
		if(day >= start)
		{
			uint16_t daysDuration = m_plantSpecies.harvestData->stepsDuration / Config::stepsPerDay;
			if(day - start < daysDuration)
				setQuantityToHarvest();
		}
	}
}
Plant::Plant(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	HasShape(data, deserializationMemo), 
	m_reservable(1), m_growthEvent(area.m_simulation.m_eventSchedule),
	m_shapeGrowthEvent(area.m_simulation.m_eventSchedule), m_fluidEvent(area.m_simulation.m_eventSchedule), 
	m_temperatureEvent(area.m_simulation.m_eventSchedule), m_endOfHarvestEvent(area.m_simulation.m_eventSchedule),
       	m_foliageGrowthEvent(area.m_simulation.m_eventSchedule),
	m_plantSpecies(*data["species"].get<const PlantSpecies*>()), 
	m_quantityToHarvest(data["harvestableQuantity"].get<uint32_t>()), m_percentGrown(data["percentGrown"].get<Percent>()), 
	m_percentFoliage(data["percentFoliage"].get<Percent>()), m_volumeFluidRequested(data["volumeFluidRequested"].get<Volume>())
{
	setLocation(data["location"].get<BlockIndex>(), &area);
	if(data.contains("fluidSource"))
		m_fluidSource = data["fluidSource"].get<BlockIndex>();
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
}
Json Plant::toJson() const
{
	Json data = HasShape::toJson();
	if(m_fluidSource)
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
void to_json(Json& data, const Plant* const& plant){ data = plant->m_location; }
void Plant::die()
{
	auto& blocks = m_area->getBlocks();
	m_growthEvent.maybeUnschedule();
	m_fluidEvent.maybeUnschedule();
	m_temperatureEvent.maybeUnschedule();
	m_endOfHarvestEvent.maybeUnschedule();
	m_foliageGrowthEvent.maybeUnschedule();
	blocks.farm_removeAllHarvestDesignations(m_location);
	blocks.farm_removeAllGiveFluidDesignations(m_location);
	BlockIndex location = m_location;
	m_area->m_hasPlants.erase(*this);
	blocks.farm_maybeDesignateForSowingIfPartOfFarmField(location);
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
	m_area->getBlocks().farm_removeAllGiveFluidDesignations(m_location);
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
		m_volumeFluidRequested = std::max(1u, util::scaleByPercent(m_plantSpecies.volumeFluidConsumed, getGrowthPercent()));
		stepsTillNextFluidEvent = m_plantSpecies.stepsTillDieWithoutFluid;
		m_area->getBlocks().farm_designateForGiveFluidIfPartOfFarmField(m_location, *this);
	}
	m_fluidEvent.maybeUnschedule();
	m_fluidEvent.schedule(stepsTillNextFluidEvent, *this);
	updateGrowingStatus();
}
void Plant::addFluid(Volume volume, [[maybe_unused]] const FluidType& fluidType)
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
	auto& blocks = m_area->getBlocks();
	if(m_fluidSource != BLOCK_INDEX_MAX && blocks.fluid_contains(m_fluidSource, m_plantSpecies.fluidType))
		return true;
	m_fluidSource = BLOCK_INDEX_MAX;
	for(BlockIndex block : blocks.collectAdjacentsInRange(m_location, getRootRange()))
		if(blocks.fluid_contains(block, m_plantSpecies.fluidType))
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
	Step duration = m_plantSpecies.harvestData->stepsDuration;
	Step now = m_area->m_simulation.m_step;
	uint16_t year = m_area->m_simulation.getDateTime().year;
	Step start = ((m_plantSpecies.harvestData->dayOfYearToStart - 1) * Config::stepsPerDay) + (year * Config::stepsPerYear);
	Step end = start + duration;
	Step remaining = end - now;
	//TODO: use of days exploitable?
	m_endOfHarvestEvent.schedule(remaining, *this);
	m_quantityToHarvest = util::scaleByPercent(m_plantSpecies.harvestData->itemQuantity, getGrowthPercent());
	m_area->getBlocks().farm_designateForHarvestIfPartOfFarmField(m_location, *this);
}
void Plant::harvest(uint32_t quantity)
{
	assert(quantity <= m_quantityToHarvest);
	m_quantityToHarvest -= quantity;
	if(m_quantityToHarvest == 0)
		endOfHarvest();
}
void Plant::endOfHarvest()
{
	m_area->getBlocks().farm_removeAllHarvestDesignations(m_location);
	if(m_plantSpecies.annual)
		die();
	else
		m_quantityToHarvest = 0;
}
void Plant::updateGrowingStatus()
{
	if(m_percentGrown == 100)
		return;
	auto& blocks = m_area->getBlocks();
	if(
			m_volumeFluidRequested == 0 && 
			blocks.isExposedToSky(m_location) == m_plantSpecies.growsInSunLight && 
			!m_temperatureEvent.exists() && 
			getPercentFoliage() >= Config::minimumPercentFoliageForGrow
	)
	{
		if(!m_growthEvent.exists())
		{
			// Start growing.
			Step delay = (m_percentGrown == 0 ?
					m_plantSpecies.stepsTillFullyGrown :
					util::scaleByPercent(m_plantSpecies.stepsTillFullyGrown, m_percentGrown));
			m_growthEvent.schedule(delay, *this);
			if((m_shape != m_plantSpecies.shapes.back() && !m_wildGrowth) || m_wildGrowth < m_plantSpecies.maxWildGrowth)
				m_shapeGrowthEvent.schedule(stepsPerShapeChange(), *this);
		}
	}
	else
	{
		if(m_growthEvent.exists())
		{
			// Stop growing.
			m_percentGrown = getGrowthPercent();
			m_growthEvent.unschedule();
			m_shapeGrowthEvent.maybeUnschedule();
			// Run updateShape here in case we are overdue, for example because the update shape event keeps getting canceled but the growth percentage has accumulated enough anyway.
			updateShape();
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
Step Plant::stepsPerShapeChange() const
{
	uint32_t shapesCount = m_plantSpecies.shapes.size() + m_plantSpecies.maxWildGrowth;
	return m_plantSpecies.stepsTillFullyGrown / shapesCount;
}
void Plant::setLocation(BlockIndex block, Area* area)
{
	if(area && area != m_area)
		m_area = area;
	m_area->getBlocks().shape_enter(block, *this);
	for(BlockIndex b: m_blocks)
		area->getBlocks().plant_set(b, *this);
}
void Plant::exit()
{
	auto& blocks = m_area->getBlocks();
	for(BlockIndex block : m_blocks)
		blocks.plant_erase(block);
	blocks.shape_exit(m_location, *this);
}
const MoveType& Plant::getMoveType() const { assert(false); return MoveType::byName("fly"); }
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
	auto& blocks = m_area->getBlocks();
	m_wildGrowth += count;
	assert(m_wildGrowth <= m_plantSpecies.maxWildGrowth);
	Simulation& simulation = m_area->m_simulation;
	while(count)
	{
		count--;
		std::vector<BlockIndex> candidates;
		for(BlockIndex block : getAdjacentBlocks())
			if(blocks.shape_anythingCanEnterEver(block) && blocks.shape_getDynamicVolume(block) == 0 && blocks.getZ(block) > blocks.getZ(m_location))
				candidates.push_back(block);
		if(candidates.empty())
			m_wildGrowth = m_plantSpecies.maxWildGrowth;
		else
		{
			BlockIndex toGrowInto = candidates[simulation.m_random.getInRange(0u, (uint)candidates.size() - 1u)];
			std::array<int32_t, 3> offset = blocks.relativeOffsetTo(m_location, toGrowInto);
			// Use the volume of the location position as the volume of the new growth position.
			std::array<int32_t, 4> position = {offset[0], offset[1], offset[2], (int)m_shape->getCollisionVolumeAtLocationBlock()};
			setShape(simulation.m_shapes.mutateAdd(*m_shape, position));
		}
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
void Plant::updateShape()
{
	Percent percent = getGrowthPercent();
	const Shape& shape = m_plantSpecies.shapeForPercentGrown(percent);
	if(&shape != m_shape)
		setShape(shape);
	uint8_t wildGrowthSteps = m_plantSpecies.wildGrowthForPercentGrown(percent);
	if(wildGrowthSteps > m_wildGrowth)
	{
		assert(m_wildGrowth + 1u == wildGrowthSteps);
		doWildGrowth();
	}
}
Step Plant::getStepAtWhichPlantWillDieFromLackOfFluid() const
{
	assert(m_volumeFluidRequested != 0);
	return m_fluidEvent.getStep();
}
// Events.
PlantGrowthEvent::PlantGrowthEvent(const Step delay, Plant& p, Step start) : 
	ScheduledEvent(p.m_area->m_simulation, delay, start), m_plant(p) {}
PlantShapeGrowthEvent::PlantShapeGrowthEvent(const Step delay, Plant& p, Step start) : 
	ScheduledEvent(p.m_area->m_simulation, delay, start), m_plant(p) {}
PlantFoliageGrowthEvent::PlantFoliageGrowthEvent(const Step delay, Plant& p, Step start) : 
	ScheduledEvent(p.m_area->m_simulation, delay, start), m_plant(p) {}
PlantEndOfHarvestEvent::PlantEndOfHarvestEvent(const Step delay, Plant& p, Step start) :
	ScheduledEvent(p.m_area->m_simulation, delay, start), m_plant(p) {}
PlantFluidEvent::PlantFluidEvent(const Step delay, Plant& p, Step start) :
	ScheduledEvent(p.m_area->m_simulation, delay, start), m_plant(p) {}
PlantTemperatureEvent::PlantTemperatureEvent(const Step delay, Plant& p, Step start) :
	ScheduledEvent(p.m_area->m_simulation, delay, start), m_plant(p) {}
Plant& AreaHasPlants::emplace(BlockIndex location, const PlantSpecies& species, const Shape* shape, Percent percentGrowth, Volume volumeFluidRequested, Step needsFluidEventStart, bool temperatureIsUnsafe, Step unsafeTemperatureEventStart, uint32_t harvestableQuantity, Percent percentFoliage)
{
	auto& blocks = m_area.getBlocks();
	assert(blocks.plant_canGrowHereEver(location, species));
	assert(!blocks.plant_exists(location));
	Plant& plant = m_plants.emplace_back(m_area, location, species, shape, percentGrowth, volumeFluidRequested, needsFluidEventStart, temperatureIsUnsafe, unsafeTemperatureEventStart, harvestableQuantity, percentFoliage);
	// TODO: plants above ground but under roof?
	if(!blocks.isUnderground(location))
		m_plantsOnSurface.insert(&plant);
	return plant;
}
Plant& AreaHasPlants::emplace(const Json& data, DeserializationMemo& deserializationMemo)
{
	Plant& plant = m_plants.emplace_back(data, deserializationMemo, m_area);
	auto& blocks = m_area.getBlocks();
	if(!blocks.isUnderground(plant.m_location))
		m_plantsOnSurface.insert(&plant);
	return plant;
}
void AreaHasPlants::erase(Plant& plant)
{
	auto& blocks = m_area.getBlocks();
	assert(blocks.plant_get(plant.m_location) == plant);
	plant.exit();
	//TODO: store iterator in plant.
	auto found = std::ranges::find(m_plants, plant);
	assert(found != m_plants.end());
	m_plants.erase(found);
	m_plantsOnSurface.erase(&plant);
}
void AreaHasPlants::onChangeAmbiantSurfaceTemperature()
{
	auto& blocks = m_area.getBlocks();
	for(Plant* plant : m_plantsOnSurface)
		plant->setTemperature(blocks.temperature_get(plant->m_location));
}
