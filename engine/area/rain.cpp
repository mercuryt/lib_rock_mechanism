#include "rain.h"
#include "fluidType.h"
#include "area.h"
#include "config.h"
#include "random.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
AreaHasRain::AreaHasRain(Area& a, Simulation&) : 
	m_humidityBySeason({30,15,10,20}),
	m_event(a.m_eventSchedule), 
	m_area(a), 
	m_defaultRainFluidType(FluidType::byName("water")) { }
void AreaHasRain::load(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo)
{
	if(data.contains("currentFluidType"))
	{
		m_currentlyRainingFluidType = data["currentFluidType"].get<const FluidType*>();
		m_intensityPercent = data["intensityPercent"].get<Percent>();
	}
	if(data.contains("eventStart"))
		m_event.schedule(data["eventDuration"].get<Step>(), *this, data["eventStart"].get<Step>());
	m_humidityBySeason = data["humdityBySeason"].get<std::array<Percent, 4>>();
}
Json AreaHasRain::toJson() const
{
	Json data;
	if(m_event.exists())
	{
		data["eventStart"] = m_event.getStartStep();
		data["eventDuration"] = m_event.duration();
	}
	if(m_currentlyRainingFluidType)
	{
		data["currentFluidType"] = m_currentlyRainingFluidType;
		data["intensityPercent"] = m_intensityPercent;
	}
	data["humdityBySeason"] = m_humidityBySeason;
	return data;
}
void AreaHasRain::start(const FluidType& fluidType, Percent intensityPercent, Step stepsDuration)
{
	assert(intensityPercent <= 100);
	assert(intensityPercent > 0);
	m_event.maybeUnschedule();
	m_currentlyRainingFluidType = &fluidType;
	m_intensityPercent = intensityPercent;
	Blocks& blocks = m_area.getBlocks();
	Plants& plants = m_area.m_plants;
	for(PlantIndex plant : plants.getPlantsOnSurface())
		if(plants.getSpecies(plant).fluidType == fluidType && blocks.isExposedToSky(plants.getLocation(plant)))
			plants.setHasFluidForNow(plant);
	m_event.schedule(stepsDuration, *this);
}
void AreaHasRain::schedule(Step restartAt) { m_event.schedule(restartAt, m_area.m_simulation, *this); }
void AreaHasRain::stop()
{
	m_currentlyRainingFluidType = nullptr;
	m_intensityPercent = 0;
}
void AreaHasRain::doStep()
{
	assert(m_intensityPercent <= 100);
	assert(m_intensityPercent > 0);
	if(m_currentlyRainingFluidType == nullptr)
		return;
	auto& random = m_area.m_simulation.m_random;
	DistanceInBlocks spacing = util::scaleByInversePercent(Config::rainMaximumSpacing, m_intensityPercent);
	DistanceInBlocks offset = random.getInRange(0u, Config::rainMaximumOffset);
	DistanceInBlocks i = 0;
	Blocks& blocks = m_area.getBlocks();
	for(BlockIndex block : blocks.getZLevel(blocks.m_sizeZ - 1))
		if(offset)
			--offset;
		else if(i)
			--i;
		else
		{
			if(!blocks.solid_is(block) && blocks.fluid_canEnterCurrently(block, *m_currentlyRainingFluidType))
				blocks.fluid_add(block, 1, *m_currentlyRainingFluidType);
			i = spacing;
		}
}
void AreaHasRain::scheduleRestart()
{
	auto random = m_area.m_simulation.m_random;
	Percent humidity = humidityForSeason();
	Step restartAt = (100 - humidity) * random.getInRange(Config::minimumStepsBetweenRainPerPercentHumidity, Config::maximumStepsBetweenRainPerPercentHumidity);
	schedule(restartAt);
}
void AreaHasRain::disable()
{
	m_event.unschedule();
}
Percent AreaHasRain::humidityForSeason() { return m_humidityBySeason[DateTime::toSeason(m_area.m_simulation.m_step)]; }
RainEvent::RainEvent(Simulation& simulation, Step delay, const Step start) : ScheduledEvent(simulation, delay, start) { }
void RainEvent::execute(Simulation&, Area* area) 
{
	if(area->m_hasRain.isRaining())
	{
		area->m_hasRain.stop(); 
		area->m_hasRain.scheduleRestart();
	}
	else
	{
		auto random = area->m_hasRain.m_area.m_simulation.m_random;
		Percent humidity = area->m_hasRain.humidityForSeason();
		Percent intensity = humidity * random.getInRange(Config::minimumRainIntensityModifier, Config::maximumRainIntensityModifier);
		Step duration = humidity * random.getInRange(Config::minimumStepsRainPerPercentHumidity, Config::maximumStepsRainPerPercentHumidity);
		assert(duration < Config::stepsPerDay);
		area->m_hasRain.start(intensity, duration);
	}
} 
void RainEvent::clearReferences(Simulation&, Area* area) { area->m_hasRain.m_event.clearPointer(); }
