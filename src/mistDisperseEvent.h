#pragma once
#include "eventSchedule.h"
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType& m_fluidType;
	Block& m_block;

	MistDisperseEvent(uint32_t s, const FluidType& ft, Block& b) :
		ScheduledEvent(s), m_fluidType(ft), m_block(b) {}
	void execute();
	bool continuesToExist() const;
};
