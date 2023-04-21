#pragma once
#include "eventSchedule.h"
class BLOCK;
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType* m_fluidType;
	BLOCK& m_block;
	MistDisperseEvent(uint32_t s, const FluidType* ft, BLOCK& b);
	void execute();
	bool continuesToExist() const;
};
