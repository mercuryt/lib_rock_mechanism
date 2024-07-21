#include "plants.h"
#include "area.h"
#include "hasShapes.h"
#include "itemType.h"
#include "plantSpecies.h"
#include "portables.h"
#include "simulation.h"
#include "types.h"

Plants::Plants(Area& area) : 
	HasShapes(area),
	m_growthEvent(area.m_eventSchedule),
	m_shapeGrowthEvent(area.m_eventSchedule),
	m_fluidEvent(area.m_eventSchedule),
	m_temperatureEvent(area.m_eventSchedule),
	m_endOfHarvestEvent(area.m_eventSchedule),
	m_foliageGrowthEvent(area.m_eventSchedule) 
{ }
void Plants::resize(HasShapeIndex newSize)
{
	HasShapes::resize(newSize);
	m_species.resize(newSize);
	m_growthEvent.resize(newSize);
	m_shapeGrowthEvent.resize(newSize);
	m_fluidEvent.resize(newSize);
	m_temperatureEvent.resize(newSize);
	m_endOfHarvestEvent.resize(newSize);
	m_foliageGrowthEvent.resize(newSize);
	m_fluidSource.resize(newSize);
	m_quantityToHarvest.resize(newSize);
	m_percentGrown.resize(newSize);
	m_percentFoliage.resize(newSize);
	m_wildGrowth.resize(newSize);
	m_volumeFluidRequested.resize(newSize);
}
void Plants::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	HasShapes::moveIndex(oldIndex, newIndex);
	m_species.at(newIndex) = m_species.at(oldIndex);
	m_growthEvent.moveIndex(oldIndex, newIndex);
	m_shapeGrowthEvent.moveIndex(oldIndex, newIndex);
	m_fluidEvent.moveIndex(oldIndex, newIndex);
	m_temperatureEvent.moveIndex(oldIndex, newIndex);
	m_endOfHarvestEvent.moveIndex(oldIndex, newIndex);
	m_foliageGrowthEvent.moveIndex(oldIndex, newIndex);
	m_fluidSource.at(newIndex) = m_fluidSource.at(oldIndex);
	m_quantityToHarvest.at(newIndex) = m_quantityToHarvest.at(oldIndex);
	m_percentGrown.at(newIndex) = m_percentGrown.at(oldIndex);
	m_percentFoliage.at(newIndex) = m_percentFoliage.at(oldIndex);
	m_wildGrowth.at(newIndex) = m_wildGrowth.at(oldIndex);
	m_volumeFluidRequested.at(newIndex) = m_volumeFluidRequested.at(oldIndex);
}
void Plants::onChangeAmbiantSurfaceTemperature()
{
	Blocks& blocks = m_area.getBlocks();
	for(PlantIndex index : m_onSurface)
	{
		Temperature temperature = blocks.temperature_get(m_location.at(index));
		setTemperature(index, temperature);
	}
}
PlantIndex Plants::create(PlantParamaters paramaters)
{
	const PlantSpecies& species = paramaters.species;
	BlockIndex location = paramaters.location;
	assert(location != BLOCK_INDEX_MAX);
	PlantIndex index = HasShapes::getNextIndex();
	assert(index < size());
	m_percentGrown.at(index) = paramaters.percentGrown == 0 ? 100 : paramaters.percentGrown;
	if(paramaters.shape == nullptr)
		paramaters.shape = &paramaters.species.shapeForPercentGrown(m_percentGrown.at(index));
	HasShapes::create(index, *paramaters.shape, location, 0, true);
	m_species.at(index) = &paramaters.species;
	m_fluidSource.at(index) = BLOCK_INDEX_MAX;
	m_quantityToHarvest.at(index) = paramaters.quantityToHarvest;
	m_percentFoliage.at(index) = 0;
	m_wildGrowth.at(index) = 0;
	m_volumeFluidRequested.at(index) = 0;
	auto& blocks = m_area.getBlocks();
	assert(blocks.plant_canGrowHereEver(location, species));
	uint8_t wildGrowth = species.wildGrowthForPercentGrown(getPercentGrown(index));
	if(wildGrowth)
		doWildGrowth(index, wildGrowth);
	// TODO: Generate event start steps from paramaters for fluid and temperature.
	m_fluidEvent.schedule(index, m_area, species.stepsNeedsFluidFrequency, index);
	Temperature temperature = blocks.temperature_get(location);
	if(temperature < species.minimumGrowingTemperature || temperature > species.maximumGrowingTemperature)
		m_temperatureEvent.schedule(index, m_area, species.stepsTillDieFromTemperature, index);
	updateGrowingStatus(index);
	m_area.m_hasFarmFields.removeAllSowSeedsDesignations(location);
	// Fruit.
	if(species.harvestData)
	{
		uint16_t day = m_area.m_simulation.getDateTime().day;
		uint16_t start = species.harvestData->dayOfYearToStart;
		if(day >= start)
		{
			uint16_t daysDuration = species.harvestData->stepsDuration / Config::stepsPerDay;
			if(day - start < daysDuration)
				setQuantityToHarvest(index);
		}
	}
	return index;
}
void Plants::die(PlantIndex index)
{
	auto& blocks = m_area.getBlocks();
	m_growthEvent.maybeUnschedule(index);
	m_fluidEvent.maybeUnschedule(index);
	m_temperatureEvent.maybeUnschedule(index);
	m_endOfHarvestEvent.maybeUnschedule(index);
	m_foliageGrowthEvent.maybeUnschedule(index);
	BlockIndex location = m_location.at(index);
	// Erase location from farm data.
	blocks.farm_removeAllHarvestDesignations(location);
	blocks.farm_removeAllGiveFluidDesignations(location);
	// Erase from blocks.
	for(BlockIndex block : getBlocks(index))
		m_area.getBlocks().plant_clearPointer(block);
	destroy(index);
	blocks.farm_maybeDesignateForSowingIfPartOfFarmField(location);
	//TODO: Create corpse and schedule rot away event.
}
void Plants::setTemperature(PlantIndex index, Temperature temperature)
{
	const PlantSpecies& species = *m_species.at(index);
	if(temperature >= species.minimumGrowingTemperature && temperature <= species.maximumGrowingTemperature)
	{
		if(m_temperatureEvent.exists(index))
			m_temperatureEvent.unschedule(index);
	}
	else
		if(!m_temperatureEvent.exists(index))
			m_temperatureEvent.schedule(index, m_area, species.stepsTillDieFromTemperature, index);
	updateGrowingStatus(index);
}
void Plants::setHasFluidForNow(PlantIndex index)
{
	m_volumeFluidRequested.at(index) = 0;
	if(m_fluidEvent.exists(index))
		m_fluidEvent.unschedule(index);
	m_fluidEvent.schedule(index, m_area, getSpecies(index).stepsNeedsFluidFrequency, index);
	updateGrowingStatus(index);
	m_area.getBlocks().farm_removeAllGiveFluidDesignations(m_location.at(index));
}
void Plants::setMaybeNeedsFluid(PlantIndex index)
{
	Step stepsTillNextFluidEvent;
	const PlantSpecies& species = *m_species.at(index);
	if(hasFluidSource(index))
	{
		m_volumeFluidRequested.at(index) = 0;
		stepsTillNextFluidEvent = species.stepsNeedsFluidFrequency;
	}
	else if(m_volumeFluidRequested.at(index) != 0)
	{
		die(index);
		return;
	}
	else // Needs fluid, stop growing and set death timer.
	{
		updateFluidVolumeRequested(index);
		stepsTillNextFluidEvent = species.stepsTillDieWithoutFluid;
		m_area.getBlocks().farm_designateForGiveFluidIfPartOfFarmField(m_location.at(index), index);
	}
	m_fluidEvent.maybeUnschedule(index);
	m_fluidEvent.schedule(index, m_area, stepsTillNextFluidEvent, index);
	updateGrowingStatus(index);
}
void Plants::addFluid(PlantIndex index, Volume volume, [[maybe_unused]] const FluidType& fluidType)
{
	assert(volume != 0);
	assert(fluidType == getSpecies(index).fluidType);
	assert(volume <= m_volumeFluidRequested.at(index));
	m_volumeFluidRequested.at(index) -= volume;
	if(m_volumeFluidRequested.at(index) == 0)
		setHasFluidForNow(index);
}
bool Plants::hasFluidSource(PlantIndex index)
{
	auto& blocks = m_area.getBlocks();
	const PlantSpecies& species = *m_species.at(index);
	if(m_fluidSource.at(index) != BLOCK_INDEX_MAX && blocks.fluid_contains(m_fluidSource.at(index), species.fluidType))
		return true;
	m_fluidSource.at(index) = BLOCK_INDEX_MAX;
	for(BlockIndex block : blocks.collectAdjacentsInRange(m_location.at(index), getRootRange(index)))
		if(blocks.fluid_contains(block, species.fluidType))
		{
			m_fluidSource.at(index) = block;
			return true;
		}
	return false;
}
void Plants::setDayOfYear(PlantIndex index, uint32_t dayOfYear)
{
	const PlantSpecies& species = *m_species.at(index);
	if(species.harvestData != nullptr && dayOfYear == species.harvestData->dayOfYearToStart)
		setQuantityToHarvest(index);
}
void Plants::setQuantityToHarvest(PlantIndex index)
{
	const PlantSpecies& species = *m_species.at(index);
	Step duration = species.harvestData->stepsDuration;
	Step now = m_area.m_simulation.m_step;
	uint16_t year = m_area.m_simulation.getDateTime().year;
	Step start = ((species.harvestData->dayOfYearToStart - 1) * Config::stepsPerDay) + (year * Config::stepsPerYear);
	Step end = start + duration;
	Step remaining = end - now;
	//TODO: use of days exploitable?
	m_endOfHarvestEvent.schedule(index, m_area, remaining, index);
	m_quantityToHarvest.at(index) = util::scaleByPercent(species.harvestData->itemQuantity, getPercentGrown(index));
	m_area.getBlocks().farm_designateForHarvestIfPartOfFarmField(m_location.at(index), index);
}
void Plants::harvest(PlantIndex index, uint32_t quantity)
{
	assert(quantity <= m_quantityToHarvest.at(index));
	m_quantityToHarvest.at(index) -= quantity;
	if(m_quantityToHarvest.at(index) == 0)
		endOfHarvest(index);
}
void Plants::endOfHarvest(PlantIndex index)
{
	m_area.getBlocks().farm_removeAllHarvestDesignations(m_location.at(index));
	if(getSpecies(index).annual)
		die(index);
	else
		m_quantityToHarvest.at(index) = 0;
}
void Plants::updateGrowingStatus(PlantIndex index)
{
	if(m_percentGrown.at(index) == 100)
		return;
	const PlantSpecies& species = *m_species.at(index);
	auto& blocks = m_area.getBlocks();
	if(
			m_volumeFluidRequested.at(index) == 0 && 
			blocks.isExposedToSky(m_location.at(index)) == species.growsInSunLight && 
			!m_temperatureEvent.exists(index) && 
			getPercentFoliage(index) >= Config::minimumPercentFoliageForGrow
	)
	{
		if(!m_growthEvent.exists(index))
		{
			// Start growing.
			Step delay = (m_percentGrown.at(index) == 0 ?
					species.stepsTillFullyGrown :
					util::scaleByPercent(species.stepsTillFullyGrown, m_percentGrown.at(index)));
			m_growthEvent.schedule(index, m_area, delay, index);
			if((m_shape.at(index) != species.shapes.back() && !m_wildGrowth.at(index)) || m_wildGrowth.at(index) < species.maxWildGrowth)
				m_shapeGrowthEvent.schedule(index, m_area, stepsPerShapeChange(index), index);
		}
	}
	else
	{
		if(m_growthEvent.exists(index))
		{
			// Stop growing.
			m_percentGrown.at(index) = getPercentGrown(index);
			m_growthEvent.unschedule(index);
			m_shapeGrowthEvent.maybeUnschedule(index);
			// Run updateShape here in case we are overdue, for example because the update shape event keeps getting canceled but the growth percentage has accumulated enough anyway.
			updateShape(index);
		}
	}
}
Percent Plants::getPercentGrown(PlantIndex index) const
{
	Percent output = m_percentGrown.at(index);
	if(m_growthEvent.exists(index))
	{
		if(m_percentGrown.at(index) != 0)
			output += (m_growthEvent.percentComplete(index) * (100u - m_percentGrown.at(index))) / 100u;
		else
			output = m_growthEvent.percentComplete(index);
	}
	return output;
}
uint32_t Plants::getRootRange(PlantIndex index) const
{
	const PlantSpecies& species = *m_species.at(index);
	return species.rootRangeMin + ((species.rootRangeMax - species.rootRangeMin) * getPercentGrown(index)) / 100;
}
Percent Plants::getPercentFoliage(PlantIndex index) const
{
	Percent output = m_percentFoliage.at(index);
	if(m_foliageGrowthEvent.exists(index))
	{
		if(m_percentFoliage.at(index) != 0)
			output += (m_foliageGrowthEvent.percentComplete(index) * (100u - m_percentFoliage.at(index))) / 100u;
		else
			output = m_foliageGrowthEvent.percentComplete(index);
	}
	return output;
}
Mass Plants::getFoliageMass(PlantIndex index) const
{
	Mass maxFoliageForType = util::scaleByPercent(getSpecies(index).adultMass, Config::percentOfPlantMassWhichIsFoliage);
	Mass maxForGrowth = util::scaleByPercent(maxFoliageForType, getPercentGrown(index));
	return util::scaleByPercent(maxForGrowth, getPercentFoliage(index));
}
void Plants::updateFluidVolumeRequested(PlantIndex index)
{
	m_volumeFluidRequested.at(index) = util::scaleByPercent(getSpecies(index).volumeFluidConsumed, getPercentGrown(index));
}
Step Plants::stepsPerShapeChange(PlantIndex index) const
{
	const PlantSpecies& species = *m_species.at(index);
	uint32_t shapesCount = species.shapes.size() + species.maxWildGrowth;
	return species.stepsTillFullyGrown / shapesCount;
}
bool Plants::temperatureEventExists(PlantIndex index) const
{
	return m_temperatureEvent.exists(index);
}
void Plants::removeFoliageMass(PlantIndex index, Mass mass)
{
	Mass maxFoliageForType = util::scaleByPercent(getSpecies(index).adultMass, Config::percentOfPlantMassWhichIsFoliage);
	Mass maxForGrowth = util::scaleByPercent(maxFoliageForType, getPercentGrown(index));
	Percent percentRemoved = std::max(1u, ((maxForGrowth - mass) / maxForGrowth) * 100u);
	m_percentFoliage.at(index) = getPercentFoliage(index);
	assert(m_percentFoliage.at(index) >= percentRemoved);
	m_percentFoliage.at(index) -= percentRemoved;
	m_foliageGrowthEvent.maybeUnschedule(index);
	makeFoliageGrowthEvent(index);
	updateGrowingStatus(index);
}
void Plants::doWildGrowth(PlantIndex index, uint8_t count)
{
	const PlantSpecies& species = *m_species.at(index);
	auto& blocks = m_area.getBlocks();
	m_wildGrowth.at(index) += count;
	assert(m_wildGrowth.at(index) <= species.maxWildGrowth);
	Simulation& simulation = m_area.m_simulation;
	while(count)
	{
		count--;
		std::vector<BlockIndex> candidates;
		for(BlockIndex block : getAdjacentBlocks(index))
			if(blocks.shape_anythingCanEnterEver(block) && blocks.shape_getDynamicVolume(block) == 0 && blocks.getZ(block) > blocks.getZ(m_location.at(index)))
				candidates.push_back(block);
		if(candidates.empty())
			m_wildGrowth.at(index) = species.maxWildGrowth;
		else
		{
			BlockIndex toGrowInto = candidates[simulation.m_random.getInRange(0u, (uint)candidates.size() - 1u)];
			std::array<int32_t, 3> offset = blocks.relativeOffsetTo(m_location.at(index), toGrowInto);
			// Use the volume of the location position as the volume of the new growth position.
			std::array<int32_t, 4> position = {offset[0], offset[1], offset[2], (int)m_shape.at(index)->getCollisionVolumeAtLocationBlock()};
			setShape(index, simulation.m_shapes.mutateAdd(*m_shape.at(index), position));
		}
	}
}
void Plants::removeFruitQuantity(PlantIndex index, uint32_t quantity)
{
	assert(quantity <= m_quantityToHarvest.at(index));
	m_quantityToHarvest.at(index) -= quantity;
}
Mass Plants::getFruitMass(PlantIndex index) const
{
	static const MaterialType& fruitType = MaterialType::byName("fruit");
	return getSpecies(index).harvestData->fruitItemType.volume * fruitType.density * m_quantityToHarvest.at(index);
}
void Plants::makeFoliageGrowthEvent(PlantIndex index)
{
	assert(m_percentFoliage.at(index) != 100);
	Step delay = util::scaleByInversePercent(getSpecies(index).stepsTillFoliageGrowsFromZero, m_percentFoliage.at(index));
	m_foliageGrowthEvent.schedule(index, m_area, delay, index);
}
void Plants::foliageGrowth(PlantIndex index)
{
	m_percentFoliage.at(index) = 100;
	updateGrowingStatus(index);
}
void Plants::updateShape(PlantIndex index)
{
	Percent percent = getPercentGrown(index);
	const PlantSpecies& species = *m_species.at(index);
	const Shape& shape = species.shapeForPercentGrown(percent);
	if(&shape != m_shape.at(index))
		setShape(index, shape);
	uint8_t wildGrowthSteps = species.wildGrowthForPercentGrown(percent);
	if(wildGrowthSteps > m_wildGrowth.at(index))
	{
		assert(m_wildGrowth.at(index) + 1u == wildGrowthSteps);
		doWildGrowth(index);
	}
}
Step Plants::getStepAtWhichPlantWillDieFromLackOfFluid(PlantIndex index) const
{
	assert(m_volumeFluidRequested.at(index) != 0);
	return m_fluidEvent.getStep(index);
}
Json Plants::toJson() const
{
	Json output;
	to_json(output, static_cast<const HasShapes&>(*this));
	output.update({
		{"m_growthEvent", m_growthEvent},
		{"m_shapeGrowthEvent", m_shapeGrowthEvent},
		{"m_fluidEvent", m_fluidEvent},
		{"m_temperatureEvent", m_temperatureEvent},
		{"m_endOfHarvestEvent", m_endOfHarvestEvent},
		{"m_foliageGrowthEvent", m_foliageGrowthEvent},
		{"m_species", m_species},
		{"m_fluidSource", m_fluidSource},
		{"m_quantityToHarvest", m_quantityToHarvest},
		{"m_percentGrown", m_percentGrown},
		{"m_percentFoliage", m_percentFoliage},
		{"m_wildGrowth", m_wildGrowth},
		{"m_volumeFluidRequested", m_volumeFluidRequested},
	});
	return output;
}
void Plants::load(const Json& data)
{
	nlohmann::from_json(data, static_cast<HasShapes&>(*this));
	m_growthEvent.load(m_area.m_simulation, data["m_growthEvent"]);
	m_shapeGrowthEvent.load(m_area.m_simulation, data["m_shapeGrowthEvent"]);
	m_fluidEvent.load(m_area.m_simulation, data["m_fluidEvent"]);
	m_temperatureEvent.load(m_area.m_simulation, data["m_temperatureEvent"]);
	m_endOfHarvestEvent.load(m_area.m_simulation, data["m_endOfHarvestEvent"]);
	m_foliageGrowthEvent.load(m_area.m_simulation, data["m_foliageGrowthEvent"]);
	m_species = data["m_species"].get<std::vector<const PlantSpecies*>>();
	m_fluidSource = data["m_fluidSource"].get<std::vector<BlockIndex>>();
	m_quantityToHarvest = data["m_quantityToHarvest"].get<std::vector<Quantity>>();
	m_percentGrown = data["m_percentGrown"].get<std::vector<Percent>>();
	m_percentFoliage = data["m_percentFoliage"].get<std::vector<Percent>>();
	m_wildGrowth = data["m_wildGrowth"].get<std::vector<uint8_t>>();
	m_volumeFluidRequested = data["m_volumeFluidRequested"].get<std::vector<CollisionVolume>>();
}
void to_json(Json& data, const Plants& plants)
{
	data = plants.toJson();
}
void Plants::log(PlantIndex index) const { std::cout << m_species.at(index)->name << ":" << std::to_string(getPercentGrown(index)) << "%"; }
// Events.
PlantGrowthEvent::PlantGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
void PlantGrowthEvent::execute(Simulation&, Area* area)
{
	Plants& plants = area->getPlants();
	plants.m_percentGrown.at(m_plant) = 100;
	if(plants.m_species.at(m_plant)->annual)
		plants.setQuantityToHarvest(m_plant);
}
void PlantGrowthEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_growthEvent.clearPointer(m_plant); }

PlantShapeGrowthEvent::PlantShapeGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
void PlantShapeGrowthEvent::execute(Simulation&, Area* area)
{
	Plants& plants = area->getPlants();
	plants.updateShape(m_plant);
	if(plants.getPercentGrown(m_plant) != 100)
		plants.m_shapeGrowthEvent.schedule(m_plant, *area, plants.stepsPerShapeChange(m_plant), m_plant);
}
void PlantShapeGrowthEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_shapeGrowthEvent.clearPointer(m_plant); }

PlantFoliageGrowthEvent::PlantFoliageGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
void PlantFoliageGrowthEvent::execute(Simulation&, Area* area){ area->getPlants().foliageGrowth(m_plant); }
void PlantFoliageGrowthEvent::clearReferences(Simulation&, Area* area){ area->getPlants().m_foliageGrowthEvent.clearPointer(m_plant); }

PlantEndOfHarvestEvent::PlantEndOfHarvestEvent(Area& area, const Step delay, PlantIndex p, Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
void PlantEndOfHarvestEvent::execute(Simulation&, Area* area) { area->getPlants().endOfHarvest(m_plant); }
void PlantEndOfHarvestEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_endOfHarvestEvent.clearPointer(m_plant); }

PlantFluidEvent::PlantFluidEvent(Area& area, const Step delay, PlantIndex p, Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
void PlantFluidEvent::execute(Simulation&, Area* area) { area->getPlants().setMaybeNeedsFluid(m_plant); }
void PlantFluidEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_fluidEvent.clearPointer(m_plant); }

PlantTemperatureEvent::PlantTemperatureEvent(Area& area, const Step delay, PlantIndex p, Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
void PlantTemperatureEvent::execute(Simulation&, Area* area) { area->getPlants().die(m_plant); }
void PlantTemperatureEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_temperatureEvent.clearPointer(m_plant); }

