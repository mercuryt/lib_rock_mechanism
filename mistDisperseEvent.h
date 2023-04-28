#pragma once
#include "eventSchedule.h"
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType* m_fluidType;
	DerivedBlock& m_block;
	MistDisperseEvent(uint32_t s, const FluidType* ft, DerivedBlock& b);
	void execute();
	bool continuesToExist() const;
};
