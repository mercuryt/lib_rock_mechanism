#pragma once
#include "eventSchedule.h"
#include "geometry/point3D.h"
struct FluidType;
class Area;
class Simulation;
class MistDisperseEvent : public ScheduledEvent
{
	FluidTypeId m_fluidType;
	Point3D m_point;
public:
	MistDisperseEvent(const Step& delay, Simulation& simulation, const FluidTypeId& ft, const Point3D& b);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { }
	bool continuesToExist(Area& area) const;
	static void emplace(Area& area, const Step& delay, const FluidTypeId& fluidType, const Point3D& point);
};
