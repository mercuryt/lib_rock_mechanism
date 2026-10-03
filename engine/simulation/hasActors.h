#pragma once

#include "../numericTypes/types.h"
#include "../config/config.h"

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
	[[nodiscard]] ActorId getNextId() { return ++m_nextId; }
	void registerActor(ActorId id, Actors& store, ActorIndex index);
	void removeActor(ActorId id);
	void update(ActorId id, Actors& store, ActorIndex index);
	[[nodiscard]] ActorIndex getIndexForId(ActorId id) const;
	[[nodiscard]] Area& getAreaForId(ActorId id) const;
	[[nodiscard]] const ActorDataLocation& getDataLocation(ActorId id) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasActors, m_nextId);
};
