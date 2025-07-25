#include "rain.h"
#include "../fluidType.h"
#include "../area/area.h"
#include "../config.h"
#include "../random.h"
#include "../numericTypes/types.h"
#include "../util.h"
#include "../simulation/simulation.h"
#include "../plants.h"
#include "../space/space.h"
AreaHasRain::AreaHasRain(Area& a, Simulation&) :
	m_humidityBySeason({Percent::create(30),Percent::create(15),Percent::create(10),Percent::create(20)}),
	m_event(a.m_eventSchedule),
	m_area(a),
	m_defaultRainFluidType(FluidType::byName("water"))
{ }
void AreaHasRain::load(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo)
{
	if(data.contains("currentFluidType"))
	{
		m_currentlyRainingFluidType = data["currentFluidType"].get<FluidTypeId>();
		m_intensityPercent = data["intensityPercent"].get<Percent>();
	}
	if(data.contains("eventStart"))
		m_event.schedule(data["eventDuration"].get<Step>(), deserializationMemo.m_simulation, data["eventStart"].get<Step>());
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
	if(m_currentlyRainingFluidType.exists())
	{
		data["currentFluidType"] = m_currentlyRainingFluidType;
		data["intensityPercent"] = m_intensityPercent;
	}
	data["humdityBySeason"] = m_humidityBySeason;
	return data;
}
void AreaHasRain::start(const FluidTypeId& fluidType, const Percent& intensityPercent, const Step& stepsDuration)
{
	assert(intensityPercent <= 100);
	assert(intensityPercent > 0);
	m_event.maybeUnschedule();
	m_currentlyRainingFluidType = fluidType;
	m_intensityPercent = intensityPercent;
	Space& space = m_area.getSpace();
	Plants& plants = m_area.getPlants();
	for(PlantIndex plant : plants.getOnSurface())
		if(PlantSpecies::getFluidType(plants.getSpecies(plant)) == fluidType && space.isExposedToSky(plants.getLocation(plant)))
			plants.setHasFluidForNow(plant);
	m_event.schedule(stepsDuration, m_area.m_simulation);
}
void AreaHasRain::schedule(Step restartAt) { m_event.schedule(restartAt, m_area.m_simulation); }
void AreaHasRain::stop()
{
	m_currentlyRainingFluidType.clear();
	m_intensityPercent = Percent::create(0);
}
void AreaHasRain::doStep()
{
	assert(m_intensityPercent <= 100);
	assert(m_intensityPercent > 0);
	if(m_currentlyRainingFluidType.empty())
		return;
	auto& random = m_area.m_simulation.m_random;
	Distance spacing = Distance::create(util::scaleByInversePercent(Config::rainMaximumSpacing.get(), m_intensityPercent));
	Distance offset = Distance::create(random.getInRange(0u, Config::rainMaximumOffset.get()));
	Distance i = Distance::create(0);
	Space& space = m_area.getSpace();
	Cuboid cuboid = space.boundry().getFace(Facing6::Above);
	for(const Point3D& point : cuboid)
	{
		if(offset != 0)
			--offset;
		else if(i != 0)
			--i;
		else
		{
			Point3D current = point;
			while(!space.solid_is(current) && space.fluid_canEnterCurrently(current, m_currentlyRainingFluidType))
			{
				current = current.below();
				if(current.z() == 0)
					break;
			}
			if(current != point)
				space.fluid_add(current.above(), CollisionVolume::create(1), m_currentlyRainingFluidType);
			i = spacing;
		}
	}
}
void AreaHasRain::scheduleRestart()
{
	auto random = m_area.m_simulation.m_random;
	Percent humidity = humidityForSeason();
	Step restartAt =
		Step::create((100 - humidity.get()) *
		random.getInRange(
			Config::minimumStepsBetweenRainPerPercentHumidity.get(),
			Config::maximumStepsBetweenRainPerPercentHumidity.get())
		);
	schedule(restartAt);
}
void AreaHasRain::disable()
{
	m_event.unschedule();
}
Percent AreaHasRain::humidityForSeason() { return m_humidityBySeason[DateTime::toSeason(m_area.m_simulation.m_step)]; }
RainEvent::RainEvent(const Step& delay, Simulation& simulation, const Step start) : ScheduledEvent(simulation, delay, start) { }
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
		float modifier = random.getInRange(Config::minimumRainIntensityModifier, Config::maximumRainIntensityModifier);
		assert(modifier != 0.0f);
		Percent intensity = Percent::create((float)humidity.get() * modifier);
		assert(intensity != 0);
		Step duration = Step::create(
			humidity.get() *
			random.getInRange(
				Config::minimumStepsRainPerPercentHumidity.get(),
				Config::maximumStepsRainPerPercentHumidity.get()
			)
		);
		assert(duration < Config::stepsPerDay);
		area->m_hasRain.start(intensity, duration);
	}
}
void RainEvent::clearReferences(Simulation&, Area* area) { area->m_hasRain.m_event.clearPointer(); }
