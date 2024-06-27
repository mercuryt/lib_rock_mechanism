#pragma once
#include "eventSchedule.h"
struct FluidType;
class Area;
class Simulation;
class MistDisperseEvent : public ScheduledEvent
{
	const FluidType& m_fluidType;
	BlockIndex m_block;
public:
	MistDisperseEvent(Simulation& simulation, uint32_t delay, const FluidType& ft, BlockIndex b);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation&, Area*) { }
	bool continuesToExist(Area& area) const;
	static void emplace(Area& area, uint32_t delay, const FluidType& fluidType, BlockIndex block);
};
