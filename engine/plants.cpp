#include "plants.h"
#include "area/area.h"
#include "eventSchedule.h"
#include "hasShapes.hpp"
#include "numericTypes/index.h"
#include "definitions/itemType.h"
#include "definitions/plantSpecies.h"
#include "portables.h"
#include "reference.h"
#include "simulation/simulation.h"
#include "numericTypes/types.h"

Plants::Plants(Area& area) :
	HasShapes<Plants, PlantIndex>(area),
	m_growthEvent(area.m_eventSchedule),
	m_shapeGrowthEvent(area.m_eventSchedule),
	m_fluidEvent(area.m_eventSchedule),
	m_temperatureEvent(area.m_eventSchedule),
	m_endOfHarvestEvent(area.m_eventSchedule),
	m_foliageGrowthEvent(area.m_eventSchedule)
{ }
void Plants::moveIndex(const PlantIndex& oldIndex, const PlantIndex& newIndex)
{
	forEachData([&](auto& data){ data.moveIndex(oldIndex, newIndex);});
	Space& space = m_area.getSpace();
	for(Point3D point : m_occupied[newIndex])
	{
		assert(space.plant_get(point) == oldIndex);
		space.plant_set(point, newIndex);
	}
}
void Plants::onChangeAmbiantSurfaceTemperature()
{
	Space& space = m_area.getSpace();
	for(const PlantIndex& index : m_onSurface)
	{
		Temperature temperature = space.temperature_get(m_location[index]);
		setTemperature(PlantIndex::cast(index), temperature);
	}
}
template<typename Action>
void Plants::forEachData(Action&& action)
{
	forEachDataHasShapes(action);
	action(m_growthEvent);
	action(m_shapeGrowthEvent);
	action(m_fluidEvent);
	action(m_temperatureEvent);
	action(m_endOfHarvestEvent);
	action(m_foliageGrowthEvent);
	action(m_species);
	action(m_fluidSource);
	action(m_quantityToHarvest);
	action(m_percentGrown);
	action(m_percentFoliage);
	action(m_wildGrowth);
	action(m_volumeFluidRequested);
	action(m_onSurface);
}
PlantIndex Plants::create(PlantParamaters paramaters)
{
	PlantSpeciesId species = paramaters.species;
	Point3D location = paramaters.location;
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
	auto& space = m_area.getSpace();
	assert(space.plant_canGrowHereEver(location, species));
	setLocation(index, location, Facing4::North);
	uint8_t wildGrowth = PlantSpecies::wildGrowthForPercentGrown(species, getPercentGrown(index));
	if(wildGrowth)
		doWildGrowth(index, wildGrowth);
	// TODO: Generate event start steps from paramaters for fluid and temperature.
	m_fluidEvent.schedule(index, m_area, PlantSpecies::getStepsNeedsFluidFrequency(species), index);
	Temperature temperature = space.temperature_get(location);
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
	if(m_area.getSpace().isExposedToSky(paramaters.location))
		m_onSurface.set(index);
	++m_sortEntropy;
	return index;
}
void Plants::destroy(const PlantIndex& index)
{
	// No need to explicitly unschedule events here, destorying the event holder will do it.
	exit(index);
	const auto& s = PlantIndex::create(size() - 1);
	if(index != s)
	{
		moveIndex(s, index);
		++m_sortEntropy;
	}
	resize(s);
}
void Plants::die(const PlantIndex& index)
{
	auto& space = m_area.getSpace();
	m_growthEvent.maybeUnschedule(index);
	m_shapeGrowthEvent.maybeUnschedule(index);
	m_fluidEvent.maybeUnschedule(index);
	m_temperatureEvent.maybeUnschedule(index);
	m_endOfHarvestEvent.maybeUnschedule(index);
	m_foliageGrowthEvent.maybeUnschedule(index);
	Point3D location = m_location[index];
	// Erase location from farm data.
	space.farm_removeAllHarvestDesignations(location);
	space.farm_removeAllGiveFluidDesignations(location);
	destroy(index);
	space.farm_maybeDesignateForSowingIfPartOfFarmField(location);
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
	m_area.getSpace().farm_removeAllGiveFluidDesignations(m_location[index]);
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
		m_area.getSpace().farm_designateForGiveFluidIfPartOfFarmField(m_location[index], index);
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
	auto& space = m_area.getSpace();
	PlantSpeciesId species = m_species[index];
	if(m_fluidSource[index].exists() && space.fluid_contains(m_fluidSource[index], PlantSpecies::getFluidType(species)))
		return true;
	m_fluidSource[index].clear();
	for(Point3D point : space.collectAdjacentsInRange(m_location[index], getRootRange(index)))
		if(space.fluid_contains(point, PlantSpecies::getFluidType(species)))
		{
			m_fluidSource[index] = point;
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
	m_area.getSpace().farm_designateForHarvestIfPartOfFarmField(m_location[index], index);
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
	m_area.getSpace().farm_removeAllHarvestDesignations(m_location[index]);
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
	auto& space = m_area.getSpace();
	if(
			m_volumeFluidRequested[index] == 0 &&
			space.isExposedToSky(m_location[index]) == PlantSpecies::getGrowsInSunLight(species) &&
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
Distance Plants::getRootRange(const PlantIndex& index) const
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
	auto& space = m_area.getSpace();
	m_wildGrowth[index] += count;
	assert(m_wildGrowth[index] <= PlantSpecies::getMaxWildGrowth(species));
	Simulation& simulation = m_area.m_simulation;
	const SmallSet<OffsetAndVolume>& positions = Shape::getPositions(m_shape[index]);
	while(count)
	{
		count--;
		std::vector<Point3D> candidates;
		for(Point3D point : getAdjacentPoints(index))
			if(
				space.shape_anythingCanEnterEver(point) &&
				space.shape_getDynamicVolume(point) == 0 &&
				point.z() > m_location[index].z()
			)
			{
				Offset3D offset = m_location[index].offsetTo(point);
				if(std::ranges::find(positions.getVector(), offset, &OffsetAndVolume::offset) == positions.getVector().end())
					candidates.push_back(point);
			}
		if(candidates.empty())
			m_wildGrowth[index] = PlantSpecies::getMaxWildGrowth(species);
		else
		{
			Point3D toGrowInto = candidates[simulation.m_random.getInRange(0u, (uint)candidates.size() - 1u)];
			Offset3D offset = m_location[index].offsetTo(toGrowInto);
			// Use the volume of the location position as the volume of the new growth position.
			OffsetAndVolume offsetAndVolume = {offset, Shape::getCollisionVolumeAtLocation(m_shape[index])};
			setShape(index, Shape::mutateAdd(m_shape[index], offsetAndVolume));
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
	return ItemType::getFullDisplacement(PlantSpecies::getFruitItemType(getSpecies(index))) * MaterialType::getDensity(fruitType) * m_quantityToHarvest[index];
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
void Plants::setLocation(const PlantIndex& index, const Point3D& location, const Facing4&)
{
	assert(m_location[index].empty());
	Space& space = m_area.getSpace();
	auto& occupied = m_occupied[index];
	for(Point3D point : Shape::getPointsOccupiedAt(m_shape[index], space, location, Facing4::North))
	{
		space.plant_set(point, index);
		occupied.insert(point);
	}
	m_location[index] = location;
	m_facing[index] = Facing4::North;
	if(space.isExposedToSky(location))
		m_onSurface.set(index);
}
void Plants::exit(const PlantIndex& index)
{
	assert(m_location[index].exists());
	Space& space = m_area.getSpace();
	for(Point3D point : m_occupied[index])
		space.plant_erase(point);
	m_location[index].clear();
	m_occupied[index].clear();
}
void Plants::sortRange(const PlantIndex& begin, const PlantIndex& end)
{
	auto sortOrder = getSortOrder(begin, end);
	forEachData([&](auto& data){ data.sortRangeWithOrder(begin, end, sortOrder); });
}
void Plants::maybeIncrementalSort(const std::chrono::microseconds timeBudget)
{
	if(m_sortEntropy < Config::plantSortEntropyThreashold)
		return;
	auto startTime = util::getCurrentTimeInMicroSeconds();
	auto endTime = startTime + timeBudget;
	while(true)
	{
		auto iterationStartTime = util::getCurrentTimeInMicroSeconds();
		if(iterationStartTime >= endTime)
			break;
		auto remainder = endTime - iterationStartTime;
		auto numberOfPlantsToSort = remainder / m_averageSortTimePerPlant;
		PlantIndex endIndex = std::min(m_incrementalSortPosition + numberOfPlantsToSort, PlantIndex::create(size()));
		sortRange(m_incrementalSortPosition, endIndex);
		m_incrementalSortPosition = endIndex;
		auto iterationEndTime = util::getCurrentTimeInMicroSeconds();
		auto iterationDuration = (iterationEndTime - iterationStartTime);
		auto totalAccumulatedSortTime = m_averageSortTimePerPlant * m_averageSortTimeSampleSize;
		m_averageSortTimeSampleSize += numberOfPlantsToSort;
		m_averageSortTimePerPlant = (totalAccumulatedSortTime + iterationDuration) / m_averageSortTimeSampleSize;
	}
	if(m_incrementalSortPosition == size())
	{
		m_incrementalSortPosition = PlantIndex::create(0);
		// Set m_sortEntropy to 0 even though we don't know that some previously sorted part may now be unsorted.
		m_sortEntropy = 0;
	}
}
void Plants::setShape(const PlantIndex& index, const ShapeId& shape)
{
	Point3D location = getLocation(index);
	exit(index);
	m_shape[index] = shape;
	setLocation(index, location, Facing4::North);
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
	m_growthEvent.load(m_area.m_simulation, data["m_growthEvent"], size);
	m_shapeGrowthEvent.load(m_area.m_simulation, data["m_shapeGrowthEvent"], size);
	m_fluidEvent.load(m_area.m_simulation, data["m_fluidEvent"], size);
	m_temperatureEvent.load(m_area.m_simulation, data["m_temperatureEvent"], size);
	m_endOfHarvestEvent.load(m_area.m_simulation, data["m_endOfHarvestEvent"], size);
	m_foliageGrowthEvent.load(m_area.m_simulation, data["m_foliageGrowthEvent"], size);
	m_species = data["m_species"].get<StrongVector<PlantSpeciesId, PlantIndex>>();
	m_fluidSource = data["m_fluidSource"].get<StrongVector<Point3D, PlantIndex>>();
	m_quantityToHarvest = data["m_quantityToHarvest"].get<StrongVector<Quantity, PlantIndex>>();
	m_percentGrown = data["m_percentGrown"].get<StrongVector<Percent, PlantIndex>>();
	m_percentFoliage = data["m_percentFoliage"].get<StrongVector<Percent, PlantIndex>>();
	m_wildGrowth = data["m_wildGrowth"].get<StrongVector<uint8_t, PlantIndex>>();
	m_volumeFluidRequested = data["m_volumeFluidRequested"].get<StrongVector<CollisionVolume, PlantIndex>>();
	Space& space = m_area.getSpace();
	for(PlantIndex index : getAll())
		for(Point3D point : m_occupied[index])
			space.plant_set(point, index);
}
void to_json(Json& data, const Plants& plants)
{
	data = plants.toJson();
}
void Plants::log(const PlantIndex& index) const { std::cout << PlantSpecies::getName(m_species[index]) << ":" << std::to_string(getPercentGrown(index).get()) << "%"; }
SmallSet<PlantIndex> Plants::getAll() const
{
	// TODO: Replace with std::iota?
	SmallSet<PlantIndex> output;
	output.reserve(m_shape.size());
	for(auto i = PlantIndex::create(0); i < size(); ++i)
		output.insert(i);
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
