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
	void registerActor(const ActorId& id, Actors& store, const ActorIndex& index);
	void removeActor(const ActorId& id);
	[[nodiscard]] const ActorIndex& getIndexForId(const ActorId& id) const;
	[[nodiscard]] Area& getAreaForId(const ActorId& id) const;
	[[nodiscard]] const ActorDataLocation& getDataLocation(const ActorId& id) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasActors, m_nextId);
};
