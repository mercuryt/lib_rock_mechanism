#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../types.h"
#include <vector>
class Area;
class Actor;
class AnimalSpecies;
class AnimalsLeaveScheduledEvent;
struct DeserializationMemo;
class Block;

struct AnimalsArriveDramaArc final : public DramaArc
{
	Block* m_entranceBlock = nullptr;;
	std::vector<Actor*> m_actors;
	bool m_isActive = false;
	const AnimalSpecies* m_species = nullptr;;
	uint32_t m_quantity = 0;
	Percent m_hungerPercent = 0;
	Percent m_thristPercent = 0;
	Percent m_tiredPercent = 0;
	AnimalsArriveDramaArc(DramaEngine& engine, Area& area);
	AnimalsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void callback();
	HasScheduledEvent<AnimalsLeaveScheduledEvent> m_scheduledEvent;
private:
	void scheduleArrive();
	void scheduleDepart();
	void scheduleContinue();
	std::pair<const AnimalSpecies*, uint32_t> getSpeciesAndQuantity() const;
	static std::vector<const AnimalSpecies*> getLargeCarnivors();
	static std::vector<const AnimalSpecies*> getMediumCarnivors();
	static std::vector<const AnimalSpecies*> getSmallCarnivors();
	static std::vector<const AnimalSpecies*> getLargeHerbivors();
	static std::vector<const AnimalSpecies*> getMediumHerbivors();
	static std::vector<const AnimalSpecies*> getSmallHerbivors();
	static bool isSmall(const Shape& shape);
	static bool isMedium(const Shape& shape);
	static bool isLarge(const Shape& shape);
	//For debug.
	void begin();
};
class AnimalsLeaveScheduledEvent final : public ScheduledEvent
{
	AnimalsArriveDramaArc& m_dramaticArc;
public:
	AnimalsLeaveScheduledEvent(AnimalsArriveDramaArc& event, Simulation& simulation, Step duration, Step start = 0) : ScheduledEvent(simulation, duration, start), m_dramaticArc(event) { }
	void execute() { m_dramaticArc.callback(); }
	void clearReferences() { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};
