#pragma once
#include "../objective.h"
#include "../config.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"

struct DeserializationMemo;
class Area;
class Actors;

class MountObjective final : public Objective
{
	ActorReference m_toMount;
	bool m_pilot;
public:
	MountObjective(const ActorReference& toMount, const bool& pilot) : Objective(Config::goToPriority), m_toMount(toMount), m_pilot(pilot) { }
	MountObjective(const Json& data, DeserializationMemo& deserializationMemo, Actors& actors);
	Json toJson() const override;
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area&, const ActorIndex&) override { }
	void delay(Area&, const ActorIndex&) override { }
	void reset(Area&, const ActorIndex&) override { }
	std::string name() const override { return "mount"; }
	// For testing.
	[[maybe_unused]] const ActorReference& getToMount() const { return m_toMount; }
};