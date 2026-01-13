#pragma once
// To be used to find actors fitting criteria.
#include "../reference.h"
#include "../numericTypes/types.h"
#include "../config/config.h"
class Area;
struct DeserializationMemo;
struct ActorQuery
{
	// Store Reference rather then Index because project saves query.
	ActorReference actor;
	Mass carryWeight = Mass::create(0);
	bool checkIfSentient = false;
	bool sentient = false;
	ActorQuery() = default;
	ActorQuery(const Json& data, Area& area);
	[[nodiscard]] bool query(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] static ActorQuery makeFor(const ActorReference& a);
	[[nodiscard]] static ActorQuery makeForCarryWeight(const Mass& cw);
	[[nodiscard]] Json toJson() const;
};