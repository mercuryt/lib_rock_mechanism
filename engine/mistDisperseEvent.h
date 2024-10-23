#pragma once
#include "eventSchedule.h"
struct FluidType;
class Area;
class Simulation;
class MistDisperseEvent : public ScheduledEvent
{
	FluidTypeId m_fluidType;
	BlockIndex m_block;
public:
	MistDisperseEvent(const Step& delay, Simulation& simulation, const FluidTypeId& ft, const BlockIndex& b);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { }
	bool continuesToExist(Area& area) const;
	static void emplace(Area& area, const Step& delay, const FluidTypeId& fluidType, const BlockIndex& block);
};
