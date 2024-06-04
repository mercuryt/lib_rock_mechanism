#pragma once
#include "eventSchedule.h"
struct FluidType;
class Area;
class MistDisperseEvent : public ScheduledEvent
{
	Area& m_area;
public:
	const FluidType& m_fluidType;
	BlockIndex m_block;

	MistDisperseEvent(uint32_t delay, Area& m_area, const FluidType& ft, BlockIndex b);
	void execute();
	void clearReferences() { }
	bool continuesToExist() const;
	static void emplace(uint32_t delay, Area& area, const FluidType& fluidType, BlockIndex block);
};
