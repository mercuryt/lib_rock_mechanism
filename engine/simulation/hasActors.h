#pragma once

#include "../types.h"
#include "../config.h"

struct DeserializationMemo;
class Actors;
class Area;

struct ActorDataLocation
{
	Actors* store;
	ActorIndex index;
};

class SimulationHasActors final
{
	ActorId m_nextId = ActorId::create(0);
	// TODO: Use boost unordered_map.
	std::unordered_map<ActorId, ActorDataLocation, ActorId::Hash> m_actors;
public:
	SimulationHasActors() = default;
	Json toJson() const;
	[[nodiscard]] ActorId getNextId() { return ++m_nextId; }
	void registerActor(ActorId id, Actors& store, ActorIndex index);
	void removeActor(ActorId id);
	ActorIndex getIndexForId(ActorId id) const;
	Area& getAreaForId(ActorId id) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasActors, m_nextId);
};
