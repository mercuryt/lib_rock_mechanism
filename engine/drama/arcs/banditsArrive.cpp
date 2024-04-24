#include "banditsArrive.h"
#include "../engine.h"
#include "../../area.h"
#include "../../config.h"
#include "../../simulation.h"
#include "../../objectives/exterminate.h"
#include "actor.h"
#include "animalSpecies.h"
#include "moveType.h"
#include <utility>
BanditsArriveDramaArc::BanditsArriveDramaArc(DramaEngine& engine, Area& area) : 
	DramaArc(engine, DramaArcType::BanditsArrive, &area), m_scheduledEvent(area.m_simulation.m_eventSchedule)
{ scheduleArrive(); }
BanditsArriveDramaArc::BanditsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo) : 
	DramaArc(data, deserializationMemo),
	m_isActive(data["isActive"].get<bool>()),
	m_quantity(data["quantity"].get<uint32_t>()),
	m_scheduledEvent(m_area->m_simulation.m_eventSchedule),
	m_leader(&deserializationMemo.actorReference(data["leader"]))
{
	m_scheduledEvent.schedule(*this, m_area->m_simulation, data["start"].get<Step>(), data["duration"].get<Step>());
}
Json BanditsArriveDramaArc::toJson() const
{
	Json data = DramaArc::toJson();
	data["isActive"] = m_isActive;
	data["duration"] = m_scheduledEvent.duration();
	data["start"] = m_scheduledEvent.getStartStep();
	data["leader"] = m_leader;
	data["quantity"] = m_quantity;
	return data;
}
void BanditsArriveDramaArc::callback()
{
	auto& random = m_area->m_simulation.m_random;
	static std::vector<const AnimalSpecies*> sentientSpecies = getSentientSpecies();
	constexpr DistanceInBlocks maxBlockDistance = 10;
	if(m_isActive)
	{
		std::unordered_set<Block*> exclude;
		Block& destination = m_area->getMiddleAtGroundLevel();
		if(!m_leader)
		{
			const AnimalSpecies& species = *random.getInVector(sentientSpecies);
			Block* location = findLocationOnEdgeForNear(*species.shapes.back(), species.moveType, *m_entranceBlock, maxBlockDistance, exclude);
			exclude.insert(location);
			ActorParamaters params{
				.species=species,
				.percentGrown=random.getInRange(90,100),
				.location=location,
				.hasSidearm=true,
				.hasLongarm=random.chance(0.4),
				.hasLightArmor=random.chance(0.9),
				.hasHeavyArmor=random.chance(0.3),
			};
			m_leader = &m_area->m_simulation.createActor(params);
			m_quantity--;
			Faction& faction = m_area->m_simulation.createFaction(m_leader->m_name + L" bandits");
			m_leader->setFaction(&faction);
			std::unique_ptr<Objective> objective = std::make_unique<ExterminateObjective>(*m_leader, destination);
			m_leader->m_hasObjectives.addTaskToStart(std::move(objective));
			m_actors.push_back(m_leader);
		}
		// Spawn.
		while(m_quantity--)
		{
			const AnimalSpecies& species = random.chance(0.5) ? m_leader->m_species : *random.getInVector(sentientSpecies);
			Block* location = findLocationOnEdgeForNear(*species.shapes.back(), species.moveType, *m_entranceBlock, maxBlockDistance, exclude);
			if(location)
			{
				exclude.insert(location);
				Actor& actor = m_area->m_simulation.createActor(ActorParamaters{
					.species=m_leader->m_species, 
					.percentGrown=random.getInRange(70,100),
					.location=location,
					.faction=m_leader->getFaction(),
					.hasSidearm=true,
					.hasLongarm=random.chance(0.6),
					.hasLightArmor=random.chance(0.9),
					.hasHeavyArmor=random.chance(0.3),
				});
				m_actors.push_back(&actor);
				std::unique_ptr<Objective> objective = std::make_unique<ExterminateObjective>(actor, destination);
				actor.m_hasObjectives.addTaskToStart(std::move(objective));
			}
			else
			{
				scheduleContinue();
				return;
			}
		}
		if(!m_quantity)
		{
			scheduleDepart();
			m_isActive = false;
			m_entranceBlock = nullptr;
			m_leader = nullptr;
		}
	}
	else
	{
		// Find entry point.
		static const Shape& shape = Shape::byName("oneByOneFull");
		static const MoveType moveType = MoveType::byName("two legs and swim in water");
		m_entranceBlock = getEntranceToArea(*m_area, shape, moveType);
		if(!m_entranceBlock)
			scheduleArrive();
		else
		{
			//TODO: Difficulty scaling.
			m_quantity = random.getInRange(3, 10);
			m_isActive = true;
			// Reenter.
			callback();
		}
	}
}
void BanditsArriveDramaArc::scheduleArrive()
{
	assert(!m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = random.getInRange(uint32_t(150 * Config::stepsPerDay), uint32_t(2 * Config::stepsPerYear));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void BanditsArriveDramaArc::scheduleDepart()
{
	assert(m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = random.getInRange(uint32_t(1 * Config::stepsPerHour), uint32_t(5 * Config::stepsPerHour));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void BanditsArriveDramaArc::scheduleContinue()
{
	assert(m_isActive);
	Step duration = Config::stepsPerSecond;
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void BanditsArriveDramaArc::begin()
{
	m_isActive = false;
	callback();
}
