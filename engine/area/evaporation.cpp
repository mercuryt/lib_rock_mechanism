#include "evaporation.h"
#include "../area.h"
#include "../fluidType.h"
AreaHasEvaporation::AreaHasEvaporation(Area& area) :
	m_event(area.m_eventSchedule)
{ }
void AreaHasEvaporation::execute(Area& area)
{
	float rate = rateModifierForTemperature(area);
	for(auto& pair : area.m_hasTemperature.getAboveGroundFluidGroupsByMeltingPoint())
		for(FluidGroup* fluidGroup : pair.second)
		{
			float fluidTypeEvaporationRate = FluidType::getEvaporationRate(fluidGroup->m_fluidType);
			if(fluidTypeEvaporationRate == 0.f)
				continue;
			Quantity blocksOnSurface = fluidGroup->countBlocksOnSurface(area);
			CollisionVolume toRemove = std::min(CollisionVolume::create(blocksOnSurface.get() * rate), CollisionVolume::create(1));
			fluidGroup->removeFluid(area, toRemove);
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
	Temperature ambiantTemperature = area.m_hasTemperature.getAmbientSurfaceTemperature();
	return ambiantTemperature.get() * Config::rateModifierForEvaporationPerDegreeTemperature;
}
EvaporationEvent::EvaporationEvent(Area& area, const Step& delay) : ScheduledEvent(area.m_simulation, delay), m_area(area) { }
void EvaporationEvent::execute(Simulation&, Area* area){ area->m_hasEvaporation.execute(*area); }
void EvaporationEvent::clearReferences(Simulation&, Area* area){ area->m_hasEvaporation.m_event.clearPointer(); }
