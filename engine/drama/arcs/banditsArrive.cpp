#include "banditsArrive.h"
#include "../engine.h"
#include "../../area.h"
#include "../../config.h"
#include "../../simulation.h"
#include "../../objectives/exterminate.h"
#include "actors/actors.h"
#include "animalSpecies.h"
#include "blocks/blocks.h"
#include "moveType.h"
#include "simulation/hasActors.h"
#include "../../types.h"
#include <utility>
BanditsArriveDramaArc::BanditsArriveDramaArc(DramaEngine& engine, Area& area) : 
	DramaArc(engine, DramaArcType::BanditsArrive, &area), m_scheduledEvent(area.m_eventSchedule)
{ scheduleArrive(); }
BanditsArriveDramaArc::BanditsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& engine) : 
	DramaArc(data, deserializationMemo, engine),
	m_isActive(data["isActive"].get<bool>()),
	m_quantity(data["quantity"].get<Quantity>()),
	m_scheduledEvent(m_area->m_eventSchedule)
{
	m_leader.setTarget(m_area->getActors().getReferenceTarget(data["leader"].get<ActorIndex>()));
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
	constexpr DistanceInBlocks maxBlockDistance = DistanceInBlocks::create(40);
	Actors& actors = m_area->getActors();
	if(m_isActive)
	{
		BlockIndices exclude;
		BlockIndex destination = m_area->getBlocks().getCenterAtGroundLevel();
		if(!m_leader.exists())
		{
			const AnimalSpecies& species = *random.getInVector(sentientSpecies);
			BlockIndex location = findLocationOnEdgeForNear(*species.shapes.back(), species.moveType, m_entranceBlock, maxBlockDistance, exclude);
			exclude.add(location);
			ActorParamaters params{
				.species=species,
				.percentGrown=Percent::create(random.getInRange(90,100)),
				.location=location,
				.hasSidearm=true,
				.hasLongarm=random.chance(0.4),
				.hasLightArmor=random.chance(0.9),
				.hasHeavyArmor=random.chance(0.3),
			};
			m_leader = actors.create(params).toReference(*m_area);
			--m_quantity;
			ActorIndex leaderIndex = m_leader.getIndex();
			Faction& faction = m_area->m_simulation.createFaction(actors.getName(leaderIndex) + L" bandits");
			actors.setFaction(leaderIndex, faction.id);
			actors.objective_addTaskToStart(leaderIndex, std::make_unique<ExterminateObjective>(*m_area, destination));
			m_actors.add(m_leader);
		}
		// Spawn.
		while(m_quantity-- != 0)
		{
			const AnimalSpecies& species = random.chance(0.5) ? actors.getSpecies(m_leader.getIndex()) : *random.getInVector(sentientSpecies);
			BlockIndex location = findLocationOnEdgeForNear(*species.shapes.back(), species.moveType, m_entranceBlock, maxBlockDistance, exclude);
			if(location.exists())
			{
				exclude.add(location);
				ActorIndex actor = actors.create(ActorParamaters{
					.species=species, 
					.percentGrown=Percent::create(random.getInRange(70,100)),
					.location=location,
					.faction=actors.getFactionId(m_leader.getIndex()),
					.hasSidearm=true,
					.hasLongarm=random.chance(0.6),
					.hasLightArmor=random.chance(0.9),
					.hasHeavyArmor=random.chance(0.3),
				});
				m_actors.add(actor.toReference(*m_area));
				actors.objective_addTaskToStart(actor, std::make_unique<ExterminateObjective>(*m_area, destination));
			}
			else
			{
				scheduleContinue();
				return;
			}
		}
		if(m_quantity.empty())
		{
			scheduleDepart();
			m_isActive = false;
			m_entranceBlock = BlockIndex::null();
			m_leader.clear();
		}
	}
	else
	{
		// Find entry point.
		static const Shape& shape = Shape::byName("oneByOneFull");
		static const MoveType moveType = MoveType::byName("two legs and swim in water");
		m_entranceBlock = getEntranceToArea(shape, moveType);
		if(m_entranceBlock.empty())
			scheduleArrive();
		else
		{
			//TODO: Difficulty scaling.
			m_quantity = Quantity::create(random.getInRange(3, 10));
			m_isActive = true;
			// Anounce.
			std::wstring message = std::to_wstring(m_quantity.get()) + L" bandits spotted nearby.";
			m_engine.getSimulation().m_hasDialogues.createMessageBox(message, m_entranceBlock);
			// Reenter.
			callback();
		}
	}
}
void BanditsArriveDramaArc::scheduleArrive()
{
	assert(!m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = Step::create(random.getInRange((Config::stepsPerDay.get() * 150), (Config::stepsPerYear.get() * 2)));
	m_scheduledEvent.schedule(*this, m_area->m_simulation, duration);
}
void BanditsArriveDramaArc::scheduleDepart()
{
	assert(m_isActive);
	auto& random = m_area->m_simulation.m_random;
	Step duration = Step::create(random.getInRange(Config::stepsPerHour.get(), (Config::stepsPerHour.get() * 5)));
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
