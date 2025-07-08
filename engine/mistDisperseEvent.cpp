#include "mistDisperseEvent.h"
#include "config.h"
#include "area/area.h"
#include "eventSchedule.h"
#include "simulation/simulation.h"
#include "fluidType.h"
#include "numericTypes/types.h"
#include "space/space.h"

#include <memory>
MistDisperseEvent::MistDisperseEvent(const Step& delay, Simulation& simulation, const FluidTypeId& ft, const Point3D& b) :
       	ScheduledEvent(simulation, delay), m_fluidType(ft), m_point(b) {}
void MistDisperseEvent::execute(Simulation& simulation, Area* area)
{
	Space& space = area->getSpace();
	// Mist does not or cannont exist here anymore, clear and return.
	if(space.fluid_getMist(m_point).empty() || space.solid_is(m_point) || space.fluid_getTotalVolume(m_point) == Config::maxPointVolume)
	{
		space.fluid_clearMist(m_point);
		return;
	}
	// Check if mist continues to exist here.
	if(continuesToExist(*area))
	{
		// Possibly spread.
		if(space.fluid_getMistInverseDistanceToSource(m_point) > 0)
			for(const Point3D& adjacent : space.getDirectlyAdjacent(m_point))
				if(space.fluid_canEnterEver(adjacent) &&
						(
							space.fluid_getMist(adjacent).empty() ||
							FluidType::getDensity(space.fluid_getMist(adjacent)) < FluidType::getDensity(m_fluidType)
						)
				)
				{
					space.fluid_mistSetFluidTypeAndInverseDistance(adjacent, m_fluidType, space.fluid_getMistInverseDistanceToSource(m_point) - 1);
					area->m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(FluidType::getMistDuration(m_fluidType), simulation, m_fluidType, adjacent));
				}
		// Schedule next check.
		area->m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(FluidType::getMistDuration(m_fluidType), simulation, m_fluidType, m_point));
		return;
	}
	// Mist does not continue to exist here.
	space.fluid_clearMist(m_point);
}
bool MistDisperseEvent::continuesToExist(Area& area) const
{
	Space& space =  area.getSpace();
	if(space.fluid_getMist(m_point) == m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(Point3D adjacent : space.getAdjacentOnSameZLevelOnly(m_point))
		// if adjacent to falling fluid.
		if(space.fluid_contains(adjacent, m_fluidType))
		{
			Point3D belowAdjacent = adjacent.below();
			if(belowAdjacent.exists() && !space.solid_is(belowAdjacent))
				return true;
		}
	for(const Point3D& adjacent : space.getDirectlyAdjacent(m_point))
		// if adjacent to point with mist with lower distance to source.
		if(space.fluid_getMist(adjacent).exists() &&
			space.fluid_getMistInverseDistanceToSource(adjacent) > space.fluid_getMistInverseDistanceToSource(adjacent)
		)
			return true;
	return false;
}
void MistDisperseEvent::emplace(Area& area, const Step& delay, const FluidTypeId& fluidType, const Point3D& point)
{
	std::unique_ptr<ScheduledEvent> event = std::make_unique<MistDisperseEvent>(delay, area.m_simulation, fluidType, point);
	area.m_eventSchedule.schedule(std::move(event));
}
