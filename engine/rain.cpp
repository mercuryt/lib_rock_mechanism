#include "rain.h"
#include "fluidType.h"
#include "plant.h"
#include "area.h"
#include "config.h"
#include "random.h"
#include "types.h"
AreaHasRain::AreaHasRain(Area& a) : m_area(a), m_currentlyRainingFluidType(nullptr), m_defaultRainFluidType(FluidType::byName("water")), m_intensityPercent(0), m_event(a.m_simulation.m_eventSchedule), m_humidityBySeason({30,15,10,20}) { }
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
	assert(intensityPercent < 100);
	assert(intensityPercent > 0);
	m_event.maybeUnschedule();
	m_currentlyRainingFluidType = &fluidType;
	m_intensityPercent = intensityPercent;
	for(Plant* plant : m_area.m_hasPlants.getPlantsOnSurface())
		if(plant->m_plantSpecies.fluidType == fluidType && plant->m_location->m_exposedToSky)
			plant->setHasFluidForNow();
	m_event.schedule(stepsDuration, *this);
}
void AreaHasRain::stop()
{
	m_currentlyRainingFluidType = nullptr;
	m_intensityPercent = 0;
}
void AreaHasRain::writeStep()
{
	if(m_currentlyRainingFluidType != nullptr)
		for(Block& block : m_area.getZLevel(m_area.m_sizeZ - 1))
			if(m_area.m_simulation.m_random.chance((double)m_intensityPercent / (double)Config::rainFrequencyModifier))
				block.addFluid(1, *m_currentlyRainingFluidType);
}
void AreaHasRain::scheduleRestart()
{
	auto random = m_area.m_simulation.m_random;
	Percent humidity = humidityForSeason();
	Step restartAt = (99 - humidity) * random.getInRange(Config::minimumStepsBetweenRainPerPercentHumidity, Config::maximumStepsBetweenRainPerPercentHumidity);
	schedule(restartAt);
}
void AreaHasRain::disable()
{
	m_event.unschedule();
}
Percent AreaHasRain::humidityForSeason() { return m_humidityBySeason[DateTime::toSeason(m_area.m_simulation.m_step)]; }
RainEvent::RainEvent(Step delay, AreaHasRain& ahr, const Step start) : ScheduledEvent(ahr.m_area.m_simulation, delay, start), m_areaHasRain(ahr) { }
void RainEvent::execute() 
{
	if(m_areaHasRain.isRaining())
	{
		m_areaHasRain.stop(); 
		m_areaHasRain.scheduleRestart();
	}
	else
	{
		auto random = m_areaHasRain.m_area.m_simulation.m_random;
		Percent humidity = m_areaHasRain.humidityForSeason();
		Percent intensity = humidity * random.getInRange(Config::minimumRainIntensityModifier, Config::maximumRainIntensityModifier);
		Step duration = humidity * random.getInRange(Config::minimumStepsRainPerPercentHumidity, Config::maximumStepsRainPerPercentHumidity);
		assert(duration < Config::stepsPerDay);
		m_areaHasRain.start(intensity, duration);
	}
}
