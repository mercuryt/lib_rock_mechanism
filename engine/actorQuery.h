#pragma once
// To be used to find actors fitting criteria.
#include "deserializationMemo.h"
#include "types.h"
#include "config.h"
class Actor;
struct DeserializationMemo;
struct ActorQuery
{
	Actor* actor;
	Mass carryWeight;
	bool checkIfSentient;
	bool sentient;
	ActorQuery(Actor& a) : actor(&a) { }
	ActorQuery(Mass cw, bool cis, bool s) : carryWeight(cw), checkIfSentient(cis), sentient(s) { }
	ActorQuery(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool operator()(Actor& actor) const;
	[[nodiscard]] static ActorQuery makeFor(Actor& a);
	[[nodiscard]] static ActorQuery makeForCarryWeight(Mass cw);
};
