#pragma once
#include "eventSchedule.h"
struct FluidType;
class Block;
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType& m_fluidType;
	Block& m_block;

	MistDisperseEvent(uint32_t delay, const FluidType& ft, Block& b);
	void execute();
	void clearReferences() { }
	bool continuesToExist() const;
	static void emplace(uint32_t delay, const FluidType& fluidType, Block& block);
};
