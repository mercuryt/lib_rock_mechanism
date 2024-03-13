#include "../event.h"
#include "eventSchedule.h"
#include "../../engine/eventSchedule.hpp"
#include "types.h"
#include <vector>
class Area;
class Actor;
class AnimalSpecies;
class AnimalsLeaveScheduledEvent;

struct AnimalsArriveDramaticEvent final : public DramaticEvent
{
	std::vector<Block*> m_entranceBlocks;
	std::vector<Actor*> m_actors;
	static float probabilty();
	void execute(Area& area);
	HasScheduledEvent<AnimalsLeaveScheduledEvent> m_scheduledEvent;
private:
	std::vector<const AnimalSpecies*> getLargeCarnivors();
	std::vector<const AnimalSpecies*> getMediumCarnivors();
	std::vector<const AnimalSpecies*> getSmallCarnivors();
	std::vector<const AnimalSpecies*> getLargeHerbivors();
	std::vector<const AnimalSpecies*> getMediumHerbivors();
	std::vector<const AnimalSpecies*> getSmallHerbivors();
};

class AnimalsLeaveScheduledEvent final : public ScheduledEvent
{
	AnimalsArriveDramaticEvent& m_dramaticEvent;
public:
	AnimalsLeaveScheduledEvent(AnimalsArriveDramaticEvent& event, Simulation& simulation, Step duration) : ScheduledEvent(simulation, duration), m_dramaticEvent(event) { }
	void execute();
	void clearReferences() { m_dramaticEvent.m_scheduledEvent.clearPointer(); }
};
