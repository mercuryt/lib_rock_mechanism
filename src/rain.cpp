#include "rain.h"
#include "plant.h"
#include "area.h"
#include "config.h"
#include "random.h"
AreaHasRain::AreaHasRain(Area& a) : m_area(a), m_currentlyRainingFluidType(nullptr), m_intensityPercent(0), m_stopEvent(a.m_simulation.m_eventSchedule) { }
void AreaHasRain::start(const FluidType& fluidType, Percent intensityPercent, Step stepsDuration)
{
	m_currentlyRainingFluidType = &fluidType;
	m_intensityPercent = intensityPercent;
	for(Plant* plant : m_area.m_hasPlants.getPlantsOnSurface())
		if(plant->m_plantSpecies.fluidType == fluidType && plant->m_location.m_exposedToSky)
			plant->setHasFluidForNow();
	m_stopEvent.schedule(stepsDuration, *this);
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
			if(m_area.m_simulation.m_random.chance((float)m_intensityPercent / (float)Config::rainFrequencyModifier))
				block.addFluid(1, *m_currentlyRainingFluidType);
}
StopRainEvent::StopRainEvent(Step delay, AreaHasRain& ahr) : ScheduledEventWithPercent(ahr.m_area.m_simulation, delay), m_areaHasRain(ahr) { }
