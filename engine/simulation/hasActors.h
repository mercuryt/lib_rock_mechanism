#pragma once

#include "../types.h"
#include "../config.h"

struct DeserializationMemo;
class Actors;
class Area;

struct ActorDataLocation
{
	Actors& store;
	ActorIndex index;
};

class SimulationHasActors final
{
	ActorId m_nextId = 0;
	std::unordered_map<ActorId, ActorDataLocation> m_actors;
public:
	SimulationHasActors() = default;
	SimulationHasActors(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	[[nodiscard]] ActorId getNextId() { return ++m_nextId; }
	void registerActor(ActorId id, Actors& store, ActorIndex index);
	void removeActor(ActorId id);
	ActorIndex getIndexForId(ActorId id) const;
	Area& getAreaForId(ActorId id) const;
};
