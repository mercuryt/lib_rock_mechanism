#include "temperature.h"
#include "block.h"
#include "area.h"
#include "nthAdjacentOffsets.h"
#include "config.h"
#include "objective.h"
#include "simulation.h"
#include <cmath>
enum class TemperatureZone { Surface, Underground, LavaSea };
Temperature TemperatureSource::getTemperatureDeltaForRange(Temperature range)
{
	if(range == 0)
		return m_temperature;
	Temperature output = m_temperature / std::pow(range, Config::heatDisipatesAtDistanceExponent);
	if(output <= Config::heatRadianceMinimum)
		return 0;
	return output; // subtract radiance mininum from output to smooth transition at edge of affected zone?
}
void TemperatureSource::apply()
{
	m_block.m_area->m_hasTemperature.addDelta(m_block, m_temperature);
	int range = 1;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->m_area->m_hasTemperature.addDelta(*block, delta);
		++range;
	}
}
void TemperatureSource::unapply()
{
	m_block.m_area->m_hasTemperature.addDelta(m_block, m_temperature * -1);
	int range = 1;
	while(int delta = getTemperatureDeltaForRange(range))
	{
		for(Block* block : getNthAdjacentBlocks(m_block, range))
			block->m_area->m_hasTemperature.addDelta(*block, delta * -1);
		++range;
	}
}
void TemperatureSource::setTemperature(const int32_t& t)
{
	unapply();
	m_temperature = t;
	apply();
}
void AreaHasTemperature::addTemperatureSource(Block& block, const Temperature& temperature)
{
	auto pair = m_sources.try_emplace(&block, temperature, block);
	assert(pair.second);
}
void AreaHasTemperature::removeTemperatureSource(TemperatureSource& temperatureSource)
{
	assert(m_sources.contains(&temperatureSource.m_block));
	//TODO: Optimize with iterator.
	m_sources.at(&temperatureSource.m_block).unapply();
	m_sources.erase(&temperatureSource.m_block);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(Block& block)
{
	return m_sources.at(&block);
}
void AreaHasTemperature::addDelta(Block& block, int32_t delta)
{
	m_blockDeltaDeltas[&block] += delta;
}
// TODO: optimize by splitting block deltas into different structures for different temperature zones, to avoid having to rerun getAmbientTemperature?
void AreaHasTemperature::applyDeltas()
{
	std::unordered_map<Block*, int32_t> oldDeltaDeltas;
	oldDeltaDeltas.swap(m_blockDeltaDeltas);
	for(auto& [block, deltaDelta] : oldDeltaDeltas)
		block->m_blockHasTemperature.updateDelta(deltaDelta);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(const Temperature& temperature)
{
	m_ambiantSurfaceTemperature = temperature;
	m_area.m_hasPlants.onChangeAmbiantSurfaceTemperature();
	m_area.m_hasActors.onChangeAmbiantSurfaceTemperature();
	m_area.m_hasItems.onChangeAmbiantSurfaceTemperature();
	for(auto& [meltingPoint, blocks] : m_aboveGroundBlocksByMeltingPoint)
		if(meltingPoint <= temperature)
			for(Block* block : blocks)
				block->m_blockHasTemperature.melt();
		else
			break;
	for(auto& [meltingPoint, fluidGroups] : m_aboveGroundFluidGroupsByMeltingPoint)
		if(meltingPoint > temperature)
			for(FluidGroup* fluidGroup : fluidGroups)
				for(FutureFlowBlock& futureFlowBlock : fluidGroup->m_drainQueue.m_queue)
					futureFlowBlock.block->m_blockHasTemperature.freeze(fluidGroup->m_fluidType);
		else
			break;
}
void AreaHasTemperature::setAmbientSurfaceTemperatureFor(DateTime now)
{
	// TODO: Latitude and altitude.
	Temperature dailyAverage = getDailyAverageAmbientSurfaceTemperature(now);
	static Temperature maxDailySwing = 35;
	static uint32_t hottestHourOfDay = 14;
	uint32_t hoursFromHottestHourOfDay = std::abs((int32_t)hottestHourOfDay - (int32_t)now.hour);
	uint32_t halfDay = Config::hoursPerDay / 2;
	setAmbientSurfaceTemperature(dailyAverage + ((maxDailySwing * (std::min(0u, halfDay - hoursFromHottestHourOfDay))) / halfDay) - (maxDailySwing / 2));
}
void AreaHasTemperature::addMeltableSolidBlockAboveGround(Block& block)
{
	assert(!block.m_underground);
	assert(block.isSolid());
	m_aboveGroundBlocksByMeltingPoint.at(block.getSolidMaterial().meltingPoint).insert(&block);
}
// Must be run before block is set no longer solid if above ground.
void AreaHasTemperature::removeMeltableSolidBlockAboveGround(Block& block)
{
	assert(block.isSolid());
	m_aboveGroundBlocksByMeltingPoint.at(block.getSolidMaterial().meltingPoint).erase(&block);
}
Temperature AreaHasTemperature::getDailyAverageAmbientSurfaceTemperature(DateTime dateTime) const
{
	// TODO: Latitude and altitude.
	static Temperature yearlyHottestDailyAverage = 300;
	static Temperature yearlyColdestDailyAverage = 280;
	static uint32_t dayOfYearOfSolstice = Config::daysPerYear / 2;
	uint32_t daysFromSolstice = std::abs((int32_t)dateTime.day - (int32_t)dayOfYearOfSolstice);
	return yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
}
void BlockHasTemperature::updateDelta(int32_t deltaDelta)
{
	m_delta += deltaDelta;
	Temperature temperature = m_delta + getAmbientTemperature();
	if(m_block.isSolid())
	{
		auto& material = m_block.getSolidMaterial();
		if(material.burnData != nullptr && material.burnData->ignitionTemperature <= temperature && (m_block.m_fires == nullptr || !m_block.m_fires->contains(&material)))
			m_block.m_area->m_fires.ignite(m_block, material);
		else if(material.meltingPoint != 0 && material.meltingPoint <= temperature)
			m_block.m_blockHasTemperature.melt();
	}
	else
	{
		m_block.m_hasPlant.setTemperature(temperature);
		m_block.m_hasBlockFeatures.setTemperature(temperature);
		m_block.m_hasActors.setTemperature(temperature);
		m_block.m_hasItems.setTemperature(temperature);
		//TODO: FluidGroups.
	}
}
void BlockHasTemperature::freeze(const FluidType& fluidType)
{
	assert(fluidType.freezesInto != nullptr);
	static const ItemType& chunk = ItemType::byName("chunk");
	uint32_t chunkVolume = m_block.m_hasItems.getCount(chunk, *fluidType.freezesInto);
	uint32_t fluidVolume = m_block.m_fluids.at(&fluidType).first;
	if(chunkVolume + fluidVolume >= Config::maxBlockVolume)
	{
		m_block.setSolid(*fluidType.freezesInto);
		uint32_t remainder = chunkVolume + fluidVolume - Config::maxBlockVolume;
		(void)remainder;
		//TODO: add remainder to fluid group or above block.
	}
	else
		m_block.m_hasItems.addGeneric(chunk, *fluidType.freezesInto,  fluidVolume);
}
void BlockHasTemperature::melt()
{
	assert(m_block.isSolid());
	assert(m_block.getSolidMaterial().meltsInto != nullptr);
	const FluidType& fluidType = *m_block.getSolidMaterial().meltsInto;
	m_block.setNotSolid();
	m_block.addFluid(Config::maxBlockVolume, fluidType);
	m_block.m_area->clearMergedFluidGroups();
}
const Temperature& BlockHasTemperature::getAmbientTemperature() const 
{
	if(m_block.m_underground)
	{
		if(m_block.m_z <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_block.m_area->m_hasTemperature.getAmbientSurfaceTemperature();
}
Temperature BlockHasTemperature::getDailyAverageAmbientTemperature() const 
{
	if(m_block.m_underground)
	{
		if(m_block.m_z <= Config::maxZLevelForDeepAmbiantTemperature)
			return Config::deepAmbiantTemperature;
		else
			return Config::undergroundAmbiantTemperature;
	}
	return m_block.m_area->m_hasTemperature.getDailyAverageAmbientSurfaceTemperature(m_block.m_area->m_simulation.m_now);
}
//TODO: Detour locked to true for emergency moves.
GetToSafeTemperatureThreadedTask::GetToSafeTemperatureThreadedTask(GetToSafeTemperatureObjective& o) : ThreadedTask(o.m_actor.getThreadedTaskEngine()), m_objective(o), m_findsPath(o.m_actor, true) ,m_noWhereWithSafeTemperatureFound(false) { }
void GetToSafeTemperatureThreadedTask::readStep()
{
	std::function<bool(const Block&, Facing facing)> condition = [&](const Block& location, Facing facing)
	{
		for(Block* adjacent : m_objective.m_actor.getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
			if(m_objective.m_actor.m_needsSafeTemperature.isSafe(adjacent->m_blockHasTemperature.get()))
				return true;
		return false;
	};
	m_findsPath.pathToPredicate(condition);
	if(!m_findsPath.found())
	{
		m_noWhereWithSafeTemperatureFound = true;
		m_findsPath.pathToAreaEdge();
	}
}
void GetToSafeTemperatureThreadedTask::writeStep()
{
	if(!m_findsPath.found())
	{
		m_objective.m_actor.m_hasObjectives.cannotFulfillNeed(m_objective);
		return;
	}
	if(m_noWhereWithSafeTemperatureFound)
		m_objective.m_noWhereWithSafeTemperatureFound = true;
	m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath());
}
void GetToSafeTemperatureThreadedTask::clearReferences(){ m_objective.m_getToSafeTemperatureThreadedTask.clearPointer(); }
GetToSafeTemperatureObjective::GetToSafeTemperatureObjective(Actor& a) : Objective(a, Config::getToSafeTemperaturePriority), m_getToSafeTemperatureThreadedTask(a.getThreadedTaskEngine()), m_noWhereWithSafeTemperatureFound(false) { }
GetToSafeTemperatureObjective::GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_getToSafeTemperatureThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), m_noWhereWithSafeTemperatureFound(data["noWhereSafeFound"].get<bool>())
{
	if(data.contains("threadedTask"))
		m_getToSafeTemperatureThreadedTask.create(*this);
}
Json GetToSafeTemperatureObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereSafeFound"] = m_noWhereWithSafeTemperatureFound;
	if(m_getToSafeTemperatureThreadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
void GetToSafeTemperatureObjective::execute()
{
	if(m_noWhereWithSafeTemperatureFound)
	{
		if(m_actor.predicateForAnyOccupiedBlock([](const Block& block){ return block.m_isEdge; }))
			// We are at the edge and can leave.
			m_actor.leaveArea();
		else
			// No safe temperature and no escape.
			m_actor.m_hasObjectives.cannotFulfillNeed(*this);
		return;
	}
	if(m_actor.m_needsSafeTemperature.isSafeAtCurrentLocation())
		m_actor.m_hasObjectives.objectiveComplete(*this);
	else
		m_getToSafeTemperatureThreadedTask.create(*this);
}
void GetToSafeTemperatureObjective::reset() 
{ 
	cancel(); 
	m_noWhereWithSafeTemperatureFound = false; 
	m_actor.m_canReserve.clearAll();
}
GetToSafeTemperatureObjective::~GetToSafeTemperatureObjective() { m_actor.m_needsSafeTemperature.m_objectiveExists = false; }
UnsafeTemperatureEvent::UnsafeTemperatureEvent(Actor& a, const Step start) : ScheduledEvent(a.getSimulation(), a.m_species.stepsTillDieInUnsafeTemperature, start), m_actor(a) { }
void UnsafeTemperatureEvent::execute() { m_actor.die(CauseOfDeath::temperature); }
void UnsafeTemperatureEvent::clearReferences() { m_actor.m_needsSafeTemperature.m_event.clearPointer(); }
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(Actor& a) : m_actor(a), m_event(a.getEventSchedule()), m_objectiveExists(false) { }
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(const Json& data, Actor& a) : m_actor(a), m_event(a.getEventSchedule()), m_objectiveExists(data["objectiveExists"].get<bool>())
{
	if(data.contains("eventStart"))
		m_event.schedule(m_actor, data["eventStart"].get<Step>());
}
Json ActorNeedsSafeTemperature::toJson() const
{
	Json data;
	data["objectiveExists"] = m_objectiveExists;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
void ActorNeedsSafeTemperature::onChange()
{
	m_actor.m_canGrow.updateGrowingStatus();
	if(!isSafeAtCurrentLocation())
	{
		if(!m_objectiveExists)
		{
			m_objectiveExists = true;
			std::unique_ptr<Objective> objective = std::make_unique<GetToSafeTemperatureObjective>(m_actor);
			m_actor.m_hasObjectives.addNeed(std::move(objective));
		}
		if(!m_event.exists())
			m_event.schedule(m_actor);
	}
	else if(m_event.exists())
		m_event.unschedule();
}
bool ActorNeedsSafeTemperature::isSafe(Temperature temperature) const
{
	return temperature >= m_actor.m_species.minimumSafeTemperature && temperature <= m_actor.m_species.maximumSafeTemperature;
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation() const
{
	if(m_actor.m_location == nullptr)
		return true;
	return isSafe(m_actor.m_location->m_blockHasTemperature.get());
}
