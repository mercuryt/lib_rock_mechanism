#pragma once
#include "eventSchedule.h"
class Block;
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType* m_fluidType;
	Block& m_block;
	MistDisperseEvent(uint32_t s, const FluidType* ft, Block& b);
	void execute();
	bool continuesToExist() const;
};
