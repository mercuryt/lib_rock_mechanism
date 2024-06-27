#pragma once
// To be used to find actors fitting criteria.
#include "types.h"
#include "config.h"
class Area;
struct DeserializationMemo;
struct ActorQuery
{
	ActorIndex actor;
	Mass carryWeight = 0;
	bool checkIfSentient = false;
	bool sentient = false;
	ActorQuery(ActorIndex a) : actor(a) { }
	ActorQuery(Mass cw, bool cis, bool s) : carryWeight(cw), checkIfSentient(cis), sentient(s) { }
	ActorQuery(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool query(Area& area, ActorIndex actor) const;
	[[nodiscard]] static ActorQuery makeFor(ActorIndex a);
	[[nodiscard]] static ActorQuery makeForCarryWeight(Mass cw);
};
