#include "evaporation.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../fluidType.h"
AreaHasEvaporation::AreaHasEvaporation(Area& area) :
	m_event(area.m_eventSchedule)
{ }
void AreaHasEvaporation::execute(Area& area)
{
	float rate = rateModifierForTemperature(area);
	for(auto& [fluidType, fluidGroups] : area.m_hasTemperature.m_freezableFluidTypeOnSurface)
	{
		float fluidTypeEvaporationRate = FluidType::getEvaporationRate(fluidType);
		if(fluidTypeEvaporationRate != 0.f)
			for(FluidGroupId fluidGroupId : fluidGroups)
			{
				FluidGroup& fluidGroup = area.m_hasFluidGroups.byId(fluidGroupId);
				int64_t pointsOnSurface = fluidGroup.countPointsOnSurface(area);
				int64_t toRemove = std::min(int64_t(pointsOnSurface * rate), 1L);
				fluidGroup.removeFluid(area, toRemove);
			}
	}
	schedule(area);
}
void AreaHasEvaporation::schedule(Area& area)
{
	Step delay = area.m_simulation.getDelayUntillNextTimeOfDay(Config::stepsPerHour*14);
	m_event.schedule(area, delay);
}
float AreaHasEvaporation::rateModifierForTemperature(Area& area) const
{
	Temperature ambiantTemperature = area.m_hasTemperature.m_ambiant;
	return ambiantTemperature.get() * Config::rateModifierForEvaporationPerDegreeTemperature;
}
EvaporationEvent::EvaporationEvent(Area& area, const Step delay) : ScheduledEvent(area.m_simulation, delay), m_area(area) { }
void EvaporationEvent::execute(Simulation&, Area* area){ area->m_hasEvaporation.execute(*area); }
void EvaporationEvent::clearReferences(Simulation&, Area* area){ area->m_hasEvaporation.m_event.clearPointer(); }
