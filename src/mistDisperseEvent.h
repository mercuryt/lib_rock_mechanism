#pragma once
#include "eventSchedule.h"
struct FluidType;
class Block;
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType& m_fluidType;
	Block& m_block;

	MistDisperseEvent(uint32_t d, const FluidType& ft, Block& b) :
		ScheduledEvent(d), m_fluidType(ft), m_block(b) {}
	void execute();
	bool continuesToExist() const;
	static void emplace(uint32_t delay, const FluidType& fluidType, Block& block)
	{
		std::unique_ptr<ScheduledEvent> event = std::make_unique<MistDisperseEvent>(delay, fluidType, block);
		eventSchedule::schedule(std::move(event));
	}
};
