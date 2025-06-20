#pragma once
#include "eventSchedule.hpp"
#include "numericTypes/types.h"
#include "config.h"
#include <array>
class Area;
class RainEvent;
struct FluidType;
struct DeserializationMemo;
class AreaHasRain final
{
	std::array<Percent, 4> m_humidityBySeason;
	HasScheduledEvent<RainEvent> m_event;
	Area& m_area;
	FluidTypeId m_currentlyRainingFluidType;
	FluidTypeId m_defaultRainFluidType;
	Percent m_intensityPercent = Percent::create(0);
public:
	AreaHasRain(Area& a, Simulation& s);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void start(const FluidTypeId& fluidType, const Percent& intensityPercent, const Step& stepsDuration);
	void start(const Percent& intensityPercent, const Step& stepsDuration) { start(m_defaultRainFluidType, intensityPercent, stepsDuration); }
	void schedule(Step restartAt);
	void stop();
	void doStep();
	void scheduleRestart();
	void disable();
	[[nodiscard]] bool isRaining() const { return m_intensityPercent != 0; }
	[[nodiscard]] FluidTypeId getFluidType() const { assert(m_currentlyRainingFluidType.exists()); return m_currentlyRainingFluidType; }
	[[nodiscard]] Percent getIntensityPercent() const { return m_intensityPercent; }
	[[nodiscard]] Percent humidityForSeason();
	friend class RainEvent;
};
class RainEvent final : public ScheduledEvent
{
public:
	RainEvent(const Step& delay, Simulation& simulation, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
