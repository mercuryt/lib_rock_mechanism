/*
	Unlike the interval DramaArcs and those triggered by the environment, this one is triggered by the player directly.
	It may only be used with a certain frequency per area / faction.
	It grants temporary courage to all who remain in line-of-sight for it's duration.
	The player is resposible for stationing the audience.

*/
#pragma once
#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../numericTypes/types.h"
#include "../../reference.h"
class Area;
struct DeserializationMemo;
class InspirationalSpeachDramaArc final : public DramaArc
{
	SmallMap<FactionId, Step> m_cooldowns;
public:
	InspirationalSpeachDramaArc(DramaEngine& engine, Area& area) : DramaArc(engine, DramaArcType::InspirationalSpeach, &area) { }
	InspirationalSpeachDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool ready(const FactionId& faction) const;
	void begin(const ActorIndex& actor);
};