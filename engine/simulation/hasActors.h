#pragma once

#include "../actor.h"
#include "../types.h"

class Simulation;
struct DeserializationMemo;

class SimulationHasActors final
{
	Simulation& m_simulation;
	ActorId m_nextId = 0;
	std::unordered_map<ActorId, Actor> m_actors;
public:
	SimulationHasActors(Simulation& simulation) : m_simulation(simulation) { }
	SimulationHasActors(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation);
	Actor& createActor(ActorParamaters params);
	Actor& createActor(const AnimalSpecies& species, BlockIndex location, Percent percentGrown = 100);
	void destroyActor(Actor& actor);
	void clearAll();
	Actor& loadActorFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Actor& getById(ActorId id);
	[[nodiscard]] ActorId getNextId() { return ++m_nextId; }
	[[nodiscard]] Json toJson() const;
};
