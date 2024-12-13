#include "plants.h"
#include "area.h"
#include "eventSchedule.h"
#include "hasShapes.hpp"
#include "index.h"
#include "itemType.h"
#include "plantSpecies.h"
#include "portables.h"
#include "reference.h"
#include "simulation.h"
#include "types.h"

Plants::Plants(Area& area) : 
	HasShapes<Plants, PlantIndex>(area),
	m_growthEvent(area.m_eventSchedule),
	m_shapeGrowthEvent(area.m_eventSchedule),
	m_fluidEvent(area.m_eventSchedule),
	m_temperatureEvent(area.m_eventSchedule),
	m_endOfHarvestEvent(area.m_eventSchedule),
	m_foliageGrowthEvent(area.m_eventSchedule)
	{
		assert(!area.m_loaded);
	}
void Plants::resize(const PlantIndex& newSize)
{
	HasShapes<Plants, PlantIndex>::resize(newSize);
	m_growthEvent.resize(newSize);
	m_shapeGrowthEvent.resize(newSize);
	m_fluidEvent.resize(newSize);
	m_temperatureEvent.resize(newSize);
	m_endOfHarvestEvent.resize(newSize);
	m_foliageGrowthEvent.resize(newSize);
	m_species.resize(newSize);
	m_fluidSource.resize(newSize);
	m_quantityToHarvest.resize(newSize);
	m_percentGrown.resize(newSize);
	m_percentFoliage.resize(newSize);
	m_wildGrowth.resize(newSize);
	m_volumeFluidRequested.resize(newSize);
}
void Plants::moveIndex(const PlantIndex& oldIndex, const PlantIndex& newIndex)
{
	HasShapes<Plants, PlantIndex>::moveIndex(oldIndex, newIndex);
	m_growthEvent.moveIndex(oldIndex, newIndex);
	m_shapeGrowthEvent.moveIndex(oldIndex, newIndex);
	m_fluidEvent.moveIndex(oldIndex, newIndex);
	m_temperatureEvent.moveIndex(oldIndex, newIndex);
	m_endOfHarvestEvent.moveIndex(oldIndex, newIndex);
	m_foliageGrowthEvent.moveIndex(oldIndex, newIndex);
	m_species[newIndex] = m_species[oldIndex];
	m_fluidSource[newIndex] = m_fluidSource[oldIndex];
	m_quantityToHarvest[newIndex] = m_quantityToHarvest[oldIndex];
	m_percentGrown[newIndex] = m_percentGrown[oldIndex];
	m_percentFoliage[newIndex] = m_percentFoliage[oldIndex];
	m_wildGrowth[newIndex] = m_wildGrowth[oldIndex];
	m_volumeFluidRequested[newIndex] = m_volumeFluidRequested[oldIndex];
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : m_blocks[newIndex])
	{
		assert(blocks.plant_get(block) == oldIndex);
		blocks.plant_set(block, newIndex);
	}
}
void Plants::onChangeAmbiantSurfaceTemperature()
{
	Blocks& blocks = m_area.getBlocks();
	for(auto index : m_onSurface)
	{
		Temperature temperature = blocks.temperature_get(m_location[index]);
		setTemperature(PlantIndex::cast(index), temperature);
	}
}
PlantIndex Plants::create(PlantParamaters paramaters)
{
	PlantSpeciesId species = paramaters.species;
	BlockIndex location = paramaters.location;
	assert(location.exists());
	PlantIndex index = PlantIndex::create(size());
	resize(index + 1);
	m_percentGrown[index] = paramaters.percentGrown.empty() ? Percent::create(100) : paramaters.percentGrown;
	if(paramaters.shape.empty())
		paramaters.shape = PlantSpecies::shapeForPercentGrown(paramaters.species, m_percentGrown[index]);
	HasShapes<Plants, PlantIndex>::create(index, paramaters.shape, paramaters.faction, true);
	m_species[index] = paramaters.species;
	m_fluidSource[index].clear();
	m_percentFoliage[index] = paramaters.percentFoliage.empty() ? Percent::create(100) : paramaters.percentFoliage;
	m_quantityToHarvest[index] = paramaters.quantityToHarvest.empty() ? Quantity::create(0) : paramaters.quantityToHarvest;
	m_wildGrowth[index] = 0;
	m_volumeFluidRequested[index] = CollisionVolume::create(0);
	auto& blocks = m_area.getBlocks();
	assert(blocks.plant_canGrowHereEver(location, species));
	setLocation(index, location, Facing::create(0));
	uint8_t wildGrowth = PlantSpecies::wildGrowthForPercentGrown(species, getPercentGrown(index));
	if(wildGrowth)
		doWildGrowth(index, wildGrowth);
	// TODO: Generate event start steps from paramaters for fluid and temperature.
	m_fluidEvent.schedule(index, m_area, PlantSpecies::getStepsNeedsFluidFrequency(species), index);
	Temperature temperature = blocks.temperature_get(location);
	if(temperature < PlantSpecies::getMinimumGrowingTemperature(species) || temperature > PlantSpecies::getMaximumGrowingTemperature(species))
		m_temperatureEvent.schedule(index, m_area, PlantSpecies::getStepsTillDieFromTemperature(species), index);
	updateGrowingStatus(index);
	m_area.m_hasFarmFields.removeAllSowSeedsDesignations(location);
	// Fruit.
	if(PlantSpecies::getStepsDurationHarvest(species).exists())
	{
		uint16_t day = m_area.m_simulation.getDateTime().day;
		uint16_t start = PlantSpecies::getDayOfYearToStartHarvest(species);
		if(day >= start)
		{
			uint16_t daysDuration = (PlantSpecies::getStepsDurationHarvest(species) / Config::stepsPerDay).get();
			if(day - start < daysDuration)
				setQuantityToHarvest(index);
		}
	}
	if(m_area.getBlocks().isOnSurface(paramaters.location))
		m_onSurface.add(index);
	return index;
}
void Plants::destroy(const PlantIndex& index)
{
	// No need to explicitly unschedule events here, destorying the event holder will do it.
	exit(index);
	const auto& s = PlantIndex::create(size() - 1);
	if(index != s)
		moveIndex(s, index);
	resize(s);
}
void Plants::die(const PlantIndex& index)
{
	auto& blocks = m_area.getBlocks();
	m_growthEvent.maybeUnschedule(index);
	m_shapeGrowthEvent.maybeUnschedule(index);
	m_fluidEvent.maybeUnschedule(index);
	m_temperatureEvent.maybeUnschedule(index);
	m_endOfHarvestEvent.maybeUnschedule(index);
	m_foliageGrowthEvent.maybeUnschedule(index);
	BlockIndex location = m_location[index];
	// Erase location from farm data.
	blocks.farm_removeAllHarvestDesignations(location);
	blocks.farm_removeAllGiveFluidDesignations(location);
	destroy(index);
	blocks.farm_maybeDesignateForSowingIfPartOfFarmField(location);
	//TODO: Create corpse and schedule rot away event.
}
void Plants::setTemperature(const PlantIndex& index, const Temperature& temperature)
{
	PlantSpeciesId species = m_species[index];
	if(temperature >= PlantSpecies::getMinimumGrowingTemperature(species) && temperature <= PlantSpecies::getMaximumGrowingTemperature(species))
	{
		if(m_temperatureEvent.exists(index))
			m_temperatureEvent.unschedule(index);
	}
	else
		if(!m_temperatureEvent.exists(index))
			m_temperatureEvent.schedule(index, m_area, PlantSpecies::getStepsTillDieFromTemperature(species), index);
	updateGrowingStatus(index);
}
void Plants::setHasFluidForNow(const PlantIndex& index)
{
	m_volumeFluidRequested[index] = CollisionVolume::create(0);
	if(m_fluidEvent.exists(index))
		m_fluidEvent.unschedule(index);
	m_fluidEvent.schedule(index, m_area, PlantSpecies::getStepsNeedsFluidFrequency(getSpecies(index)), index);
	updateGrowingStatus(index);
	m_area.getBlocks().farm_removeAllGiveFluidDesignations(m_location[index]);
}
void Plants::setMaybeNeedsFluid(const PlantIndex& index)
{
	Step stepsTillNextFluidEvent;
	PlantSpeciesId species = m_species[index];
	if(hasFluidSource(index))
	{
		m_volumeFluidRequested[index] = CollisionVolume::create(0);
			stepsTillNextFluidEvent = PlantSpecies::getStepsNeedsFluidFrequency(species);
	}
	else if(m_volumeFluidRequested[index] != 0)
	{
		die(index);
		return;
	}
	else // Needs fluid, stop growing and set death timer.
	{
		updateFluidVolumeRequested(index);
		stepsTillNextFluidEvent = PlantSpecies::getStepsTillDieWithoutFluid(species);
		m_area.getBlocks().farm_designateForGiveFluidIfPartOfFarmField(m_location[index], index);
	}
	m_fluidEvent.maybeUnschedule(index);
	m_fluidEvent.schedule(index, m_area, stepsTillNextFluidEvent, index);
	updateGrowingStatus(index);
}
void Plants::addFluid(const PlantIndex& index, const CollisionVolume& volume, [[maybe_unused]] const FluidTypeId& fluidType)
{
	assert(volume != 0);
	assert(fluidType == PlantSpecies::getFluidType(getSpecies(index)));
	assert(volume <= m_volumeFluidRequested[index]);
	m_volumeFluidRequested[index] -= volume;
	if(m_volumeFluidRequested[index] == 0)
		setHasFluidForNow(index);
}
bool Plants::hasFluidSource(const PlantIndex& index)
{
	auto& blocks = m_area.getBlocks();
	PlantSpeciesId species = m_species[index];
	if(m_fluidSource[index].exists() && blocks.fluid_contains(m_fluidSource[index], PlantSpecies::getFluidType(species)))
		return true;
	m_fluidSource[index].clear();
	for(BlockIndex block : blocks.collectAdjacentsInRange(m_location[index], getRootRange(index)))
		if(blocks.fluid_contains(block, PlantSpecies::getFluidType(species)))
		{
			m_fluidSource[index] = block;
			return true;
		}
	return false;
}
void Plants::setDayOfYear(const PlantIndex& index, uint32_t dayOfYear)
{
	PlantSpeciesId species = m_species[index];
	if(PlantSpecies::getItemQuantityToHarvest(species).exists() && dayOfYear == PlantSpecies::getDayOfYearToStartHarvest(species))
		setQuantityToHarvest(index);
}
void Plants::setQuantityToHarvest(const PlantIndex& index)
{
	PlantSpeciesId species = m_species[index];
	Step duration = PlantSpecies::getStepsDurationHarvest(species);
	Step now = m_area.m_simulation.m_step;
	uint16_t year = m_area.m_simulation.getDateTime().year;
	Step start = (Config::stepsPerDay * (PlantSpecies::getDayOfYearToStartHarvest(species) - 1)) + (Config::stepsPerYear * year);
	Step end = start + duration;
	Step remaining = end - now;
	//TODO: use of days exploitable?
	m_endOfHarvestEvent.schedule(index, m_area, remaining, index);
	m_quantityToHarvest[index] = Quantity::create(util::scaleByPercent(PlantSpecies::getItemQuantityToHarvest(species).get(), getPercentGrown(index)));
	m_area.getBlocks().farm_designateForHarvestIfPartOfFarmField(m_location[index], index);
}
void Plants::harvest(const PlantIndex& index, const Quantity& quantity)
{
	assert(quantity <= m_quantityToHarvest[index]);
	m_quantityToHarvest[index] -= quantity;
	if(m_quantityToHarvest[index] == 0)
		endOfHarvest(index);
}
void Plants::endOfHarvest(const PlantIndex& index)
{
	m_area.getBlocks().farm_removeAllHarvestDesignations(m_location[index]);
	if(PlantSpecies::getAnnual(getSpecies(index)))
		die(index);
	else
		m_quantityToHarvest[index] = Quantity::create(0);
}
void Plants::updateGrowingStatus(const PlantIndex& index)
{
	if(m_percentGrown[index] == 100)
		return;
	PlantSpeciesId species = m_species[index];
	auto& blocks = m_area.getBlocks();
	if(
			m_volumeFluidRequested[index] == 0 && 
			blocks.isExposedToSky(m_location[index]) == PlantSpecies::getGrowsInSunLight(species) && 
			!m_temperatureEvent.exists(index) && 
			getPercentFoliage(index) >= Config::minimumPercentFoliageForGrow
	)
	{
		if(!m_growthEvent.exists(index))
		{
			// Start growing.
			Step delay = (m_percentGrown[index] == 0 ?
					PlantSpecies::getStepsTillFullyGrown(species) :
					Step::create(util::scaleByPercent(PlantSpecies::getStepsTillFullyGrown(species).get(), m_percentGrown[index])));
			m_growthEvent.schedule(index, m_area, delay, index);
			if((m_shape[index] != PlantSpecies::getShapes(species).back() && !m_wildGrowth[index]) || m_wildGrowth[index] < PlantSpecies::getMaxWildGrowth(species))
				m_shapeGrowthEvent.schedule(index, m_area, stepsPerShapeChange(index), index);
		}
	}
	else
	{
		if(m_growthEvent.exists(index))
		{
			// Stop growing.
			m_percentGrown[index] = getPercentGrown(index);
			m_growthEvent.unschedule(index);
			m_shapeGrowthEvent.maybeUnschedule(index);
			// Run updateShape here in case we are overdue, for example because the update shape event keeps getting canceled but the growth percentage has accumulated enough anyway.
			updateShape(index);
		}
	}
}
Percent Plants::getPercentGrown(const PlantIndex& index) const
{
	Percent output = m_percentGrown[index];
	if(m_growthEvent.exists(index))
	{
		if(m_percentGrown[index] != 0)
			output += (m_growthEvent.percentComplete(index) * (Percent::create(100) - m_percentGrown[index])) / 100u;
		else
			output = m_growthEvent.percentComplete(index);
	}
	return output;
}
DistanceInBlocks Plants::getRootRange(const PlantIndex& index) const
{
	PlantSpeciesId species = m_species[index];
	return PlantSpecies::getRootRangeMin(species) + util::scaleByPercentRange(PlantSpecies::getRootRangeMin(species).get(), PlantSpecies::getRootRangeMax(species).get(), getPercentGrown(index));
}
Percent Plants::getPercentFoliage(const PlantIndex& index) const
{
	Percent output = m_percentFoliage[index];
	if(m_foliageGrowthEvent.exists(index))
	{
		if(m_percentFoliage[index] != 0)
			output += (m_foliageGrowthEvent.percentComplete(index) * (Percent::create(100) - m_percentFoliage[index])) / 100u;
		else
			output = m_foliageGrowthEvent.percentComplete(index);
	}
	return output;
}
Mass Plants::getFoliageMass(const PlantIndex& index) const
{
	Mass maxFoliageForType = Mass::create(util::scaleByPercent(PlantSpecies::getAdultMass(getSpecies(index)).get(), Config::percentOfPlantMassWhichIsFoliage));
	Mass maxForGrowth = Mass::create(util::scaleByPercent(maxFoliageForType.get(), getPercentGrown(index)));
	return Mass::create(util::scaleByPercent(maxForGrowth.get(), getPercentFoliage(index)));
}
void Plants::updateFluidVolumeRequested(const PlantIndex& index)
{
	CollisionVolume requested = CollisionVolume::create(util::scaleByPercent(PlantSpecies::getVolumeFluidConsumed(getSpecies(index)).get(), getPercentGrown(index)));
	m_volumeFluidRequested[index] = requested;
}
Step Plants::stepsPerShapeChange(const PlantIndex& index) const
{
	PlantSpeciesId species = m_species[index];
	uint32_t shapesCount = PlantSpecies::getShapes(species).size() + PlantSpecies::getMaxWildGrowth(species);
	return PlantSpecies::getStepsTillFullyGrown(species) / shapesCount;
}
bool Plants::temperatureEventExists(const PlantIndex& index) const
{
	return m_temperatureEvent.exists(index);
}
bool Plants::fluidEventExists(const PlantIndex& index) const
{
	return m_fluidEvent.exists(index);
}
bool Plants::growthEventExists(const PlantIndex& index) const
{
	return m_growthEvent.exists(index);
}
void Plants::removeFoliageMass(const PlantIndex& index, const Mass& mass)
{
	Mass maxFoliageForType = Mass::create(util::scaleByPercent(PlantSpecies::getAdultMass(getSpecies(index)).get(), Config::percentOfPlantMassWhichIsFoliage));
	Mass maxForGrowth = Mass::create(util::scaleByPercent(maxFoliageForType.get(), getPercentGrown(index)));
	Percent percentRemoved = Percent::create(std::max(1u, ((maxForGrowth - mass) / maxForGrowth).get() * 100u));
	m_percentFoliage[index] = getPercentFoliage(index);
	assert(m_percentFoliage[index] >= percentRemoved);
	m_percentFoliage[index] -= percentRemoved;
	m_foliageGrowthEvent.maybeUnschedule(index);
	makeFoliageGrowthEvent(index);
	updateGrowingStatus(index);
}
void Plants::doWildGrowth(const PlantIndex& index, uint8_t count)
{
	PlantSpeciesId species = m_species[index];
	auto& blocks = m_area.getBlocks();
	m_wildGrowth[index] += count;
	assert(m_wildGrowth[index] <= PlantSpecies::getMaxWildGrowth(species));
	Simulation& simulation = m_area.m_simulation;
	while(count)
	{
		count--;
		std::vector<BlockIndex> candidates;
		for(BlockIndex block : getAdjacentBlocks(index))
			if(blocks.shape_anythingCanEnterEver(block) && blocks.shape_getDynamicVolume(block) == 0 && blocks.getZ(block) > blocks.getZ(m_location[index]))
				candidates.push_back(block);
		if(candidates.empty())
			m_wildGrowth[index] = PlantSpecies::getMaxWildGrowth(species);
		else
		{
			BlockIndex toGrowInto = candidates[simulation.m_random.getInRange(0u, (uint)candidates.size() - 1u)];
			std::array<int32_t, 3> offset = blocks.relativeOffsetTo(m_location[index], toGrowInto);
			// Use the volume of the location position as the volume of the new growth position.
			std::array<int32_t, 4> position = {offset[0], offset[1], offset[2], (int)Shape::getCollisionVolumeAtLocationBlock(m_shape[index]).get()};
			setShape(index, Shape::mutateAdd(m_shape[index], position));
		}
	}
}
void Plants::removeFruitQuantity(const PlantIndex& index, const Quantity& quantity)
{
	assert(quantity <= m_quantityToHarvest[index]);
	m_quantityToHarvest[index] -= quantity;
}
Mass Plants::getFruitMass(const PlantIndex& index) const
{
	static MaterialTypeId fruitType = MaterialType::byName("fruit");
	return ItemType::getVolume(PlantSpecies::getFruitItemType(getSpecies(index))) * MaterialType::getDensity(fruitType) * m_quantityToHarvest[index];
}
void Plants::makeFoliageGrowthEvent(const PlantIndex& index)
{
	assert(m_percentFoliage[index] != 100);
	Step delay = Step::create(util::scaleByInversePercent(PlantSpecies::getStepsTillFoliageGrowsFromZero(getSpecies(index)).get(), m_percentFoliage[index]));
	m_foliageGrowthEvent.schedule(index, m_area, delay, index);
}
void Plants::foliageGrowth(const PlantIndex& index)
{
	m_percentFoliage[index] = Percent::create(100);
	updateGrowingStatus(index);
}
Step Plants::getStepAtWhichPlantWillDieFromLackOfFluid(const PlantIndex& index) const
{
	assert(m_volumeFluidRequested[index] != 0);
	return m_fluidEvent.getStep(index);
}
void Plants::updateShape(const PlantIndex& index)
{
	Percent percent = getPercentGrown(index);
	PlantSpeciesId species = m_species[index];
	ShapeId shape = PlantSpecies::shapeForPercentGrown(species, percent);
	if(shape != m_shape[index])
		setShape(index, shape);
	uint8_t wildGrowthSteps = PlantSpecies::wildGrowthForPercentGrown(species, percent);
	if(wildGrowthSteps > m_wildGrowth[index])
	{
		assert(m_wildGrowth[index] + 1u == wildGrowthSteps);
		doWildGrowth(index);
	}
}
void Plants::setLocation(const PlantIndex& index, const BlockIndex& location, const Facing&)
{
	assert(m_location[index].empty());
	Blocks& blocks = m_area.getBlocks();
	auto& occupied = m_blocks[index];
	for(BlockIndex block : Shape::getBlocksOccupiedAt(m_shape[index], blocks, location, Facing::create(0)))
	{
		blocks.plant_set(block, index);
		occupied.add(block);
	}
	m_location[index] = location;
	m_facing[index] = Facing::create(0);
}
void Plants::exit(const PlantIndex& index)
{
	assert(m_location[index].exists());
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : m_blocks[index])
		blocks.plant_erase(block);
	m_location[index].clear();
	m_blocks[index].clear();
}
void Plants::setShape(const PlantIndex& index, const ShapeId& shape)
{
	BlockIndex location = getLocation(index);
	exit(index);
	m_shape[index] = shape;
	setLocation(index, location, Facing::create(0));
}
bool Plants::blockIsFull(const BlockIndex& block)
{
	return m_area.getBlocks().plant_exists(block);
}
PlantSpeciesId Plants::getSpecies(const PlantIndex& index) const { return m_species[index]; }
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
	PlantIndex size = PlantIndex::create(m_shape.size());
	m_growthEvent.load(m_area.m_simulation, data["m_growthEvent"], size.toHasShape());
	m_shapeGrowthEvent.load(m_area.m_simulation, data["m_shapeGrowthEvent"], size.toHasShape());
	m_fluidEvent.load(m_area.m_simulation, data["m_fluidEvent"], size.toHasShape());
	m_temperatureEvent.load(m_area.m_simulation, data["m_temperatureEvent"], size.toHasShape());
	m_endOfHarvestEvent.load(m_area.m_simulation, data["m_endOfHarvestEvent"], size.toHasShape());
	m_foliageGrowthEvent.load(m_area.m_simulation, data["m_foliageGrowthEvent"], size.toHasShape());
	m_species = data["m_species"].get<DataVector<PlantSpeciesId, PlantIndex>>();
	m_fluidSource = data["m_fluidSource"].get<DataVector<BlockIndex, PlantIndex>>();
	m_quantityToHarvest = data["m_quantityToHarvest"].get<DataVector<Quantity, PlantIndex>>();
	m_percentGrown = data["m_percentGrown"].get<DataVector<Percent, PlantIndex>>();
	m_percentFoliage = data["m_percentFoliage"].get<DataVector<Percent, PlantIndex>>();
	m_wildGrowth = data["m_wildGrowth"].get<DataVector<uint8_t, PlantIndex>>();
	m_volumeFluidRequested = data["m_volumeFluidRequested"].get<DataVector<CollisionVolume, PlantIndex>>();
	Blocks& blocks = m_area.getBlocks();
	for(PlantIndex index : getAll())
		for(BlockIndex block : m_blocks[index])
			blocks.plant_set(block, index);
}
void to_json(Json& data, const Plants& plants)
{
	data = plants.toJson();
}
void Plants::log(const PlantIndex& index) const { std::cout << PlantSpecies::getName(m_species[index]) << ":" << std::to_string(getPercentGrown(index).get()) << "%"; }
PlantIndices Plants::getAll() const
{
	// TODO: Replace with std::iota?
	PlantIndices output;
	output.reserve(m_shape.size());
	for(auto i = PlantIndex::create(0); i < size(); ++i)
		output.add(i);
	return output;
}
// Events.
PlantGrowthEvent::PlantGrowthEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
PlantGrowthEvent::PlantGrowthEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>()), m_plant(data["plant"].get<PlantIndex>()) { }
void PlantGrowthEvent::execute(Simulation&, Area* area)
{
	Plants& plants = area->getPlants();
	plants.m_percentGrown[m_plant] = Percent::create(100);
	if(PlantSpecies::getAnnual(plants.m_species[m_plant]))
		plants.setQuantityToHarvest(m_plant);
}
void PlantGrowthEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_growthEvent.clearPointer(m_plant); }
Json PlantGrowthEvent::toJson() const { return {{"delay", duration()}, {"start", m_startStep}, {"plant", m_plant}}; }

PlantShapeGrowthEvent::PlantShapeGrowthEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
PlantShapeGrowthEvent::PlantShapeGrowthEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>()), m_plant(data["plant"].get<PlantIndex>()) { }
void PlantShapeGrowthEvent::execute(Simulation&, Area* area)
{
	Plants& plants = area->getPlants();
	plants.updateShape(m_plant);
	if(plants.getPercentGrown(m_plant) != 100)
		plants.m_shapeGrowthEvent.schedule(m_plant, *area, plants.stepsPerShapeChange(m_plant), m_plant);
}
void PlantShapeGrowthEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_shapeGrowthEvent.clearPointer(m_plant); }
Json PlantShapeGrowthEvent::toJson() const { return {{"delay", duration()}, {"start", m_startStep}, {"plant", m_plant}}; }

PlantFoliageGrowthEvent::PlantFoliageGrowthEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
PlantFoliageGrowthEvent::PlantFoliageGrowthEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>()), m_plant(data["plant"].get<PlantIndex>()) { }
void PlantFoliageGrowthEvent::execute(Simulation&, Area* area){ area->getPlants().foliageGrowth(m_plant); }
void PlantFoliageGrowthEvent::clearReferences(Simulation&, Area* area){ area->getPlants().m_foliageGrowthEvent.clearPointer(m_plant); }
Json PlantFoliageGrowthEvent::toJson() const { return {{"delay", duration()}, {"start", m_startStep}, {"plant", m_plant}}; }

PlantEndOfHarvestEvent::PlantEndOfHarvestEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
PlantEndOfHarvestEvent::PlantEndOfHarvestEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>()), m_plant(data["plant"].get<PlantIndex>()) { }
void PlantEndOfHarvestEvent::execute(Simulation&, Area* area) { area->getPlants().endOfHarvest(m_plant); }
void PlantEndOfHarvestEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_endOfHarvestEvent.clearPointer(m_plant); }
Json PlantEndOfHarvestEvent::toJson() const { return {{"delay", duration()}, {"start", m_startStep}, {"plant", m_plant}}; }

PlantFluidEvent::PlantFluidEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
PlantFluidEvent::PlantFluidEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>()), m_plant(data["plant"].get<PlantIndex>()) { }
void PlantFluidEvent::execute(Simulation&, Area* area) { area->getPlants().setMaybeNeedsFluid(m_plant); }
void PlantFluidEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_fluidEvent.clearPointer(m_plant); }
Json PlantFluidEvent::toJson() const { return {{"delay", duration()}, {"start", m_startStep}, {"plant", m_plant}}; }

PlantTemperatureEvent::PlantTemperatureEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_plant(p) {}
PlantTemperatureEvent::PlantTemperatureEvent(Simulation& simulation, const Json& data) :
	ScheduledEvent(simulation, data["delay"].get<Step>(), data["start"].get<Step>()), m_plant(data["plant"].get<PlantIndex>()) { }
void PlantTemperatureEvent::execute(Simulation&, Area* area) { area->getPlants().die(m_plant); }
void PlantTemperatureEvent::clearReferences(Simulation&, Area* area) { area->getPlants().m_temperatureEvent.clearPointer(m_plant); }
Json PlantTemperatureEvent::toJson() const { return {{"delay", duration()}, {"start", m_startStep}, {"plant", m_plant}}; }
