#pragma once
// To be used to find actors fitting criteria.
#include "reference.h"
#include "types.h"
#include "config.h"
class Area;
struct DeserializationMemo;
struct ActorQuery
{
	// Store Reference rather then Index because project saves query.
	ActorReference actor;
	Mass carryWeight = Mass::create(0);
	bool checkIfSentient = false;
	bool sentient = false;
	[[nodiscard]] bool query(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] static ActorQuery makeFor(const ActorReference& a);
	[[nodiscard]] static ActorQuery makeForCarryWeight(const Mass& cw);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActorQuery, actor, carryWeight, checkIfSentient, sentient);
};
