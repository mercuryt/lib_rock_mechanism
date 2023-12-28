#pragma once
#include "deserializationMemo.h"
#include "fluidType.h"
#include "eventSchedule.hpp"
class Area;
class StopRainEvent;
class AreaHasRain final
{
	Area& m_area;
	const FluidType* m_currentlyRainingFluidType;
	Percent m_intensityPercent;
	HasScheduledEvent<StopRainEvent> m_stopEvent;
public:
	AreaHasRain(Area& a);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void start(const FluidType& fluidType, Percent intensityPercent, Step stepsDuration);
	void stop();
	void writeStep();
	friend class StopRainEvent;
};
class StopRainEvent final : public ScheduledEventWithPercent
{
	AreaHasRain& m_areaHasRain;
public:
	StopRainEvent(Step delay, AreaHasRain& ahr, const Step start = 0);
	void execute() { m_areaHasRain.stop(); }
	void clearReferences() { m_areaHasRain.m_stopEvent.clearPointer(); }
};
