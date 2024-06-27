#pragma once
#include "eventSchedule.hpp"
#include "types.h"
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
	const FluidType* m_currentlyRainingFluidType = nullptr;
	const FluidType& m_defaultRainFluidType;
	Percent m_intensityPercent = 0;
public:
	AreaHasRain(Area& a, Simulation& s);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void start(const FluidType& fluidType, Percent intensityPercent, Step stepsDuration);
	void start(Percent intensityPercent, Step stepsDuration) { start(m_defaultRainFluidType, intensityPercent, stepsDuration); }
	void schedule(Step restartAt);
	void stop();
	void doStep();
	void scheduleRestart();
	void disable();
	[[nodiscard]] bool isRaining() const { return m_intensityPercent != 0; }
	[[nodiscard]] const FluidType& getFluidType() const { assert(m_currentlyRainingFluidType); return *m_currentlyRainingFluidType; }
	[[nodiscard]] Percent getIntensityPercent() const { return m_intensityPercent; }
	[[nodiscard]] Percent humidityForSeason();
	friend class RainEvent;
};
class RainEvent final : public ScheduledEvent
{
public:
	RainEvent(Simulation& simulation, Step delay, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
