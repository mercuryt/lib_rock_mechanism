#pragma once
#include "objective.h"
#include "config.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"

struct DeserializationMemo;
class Area;

class GoToObjective final : public Objective
{
	BlockIndex m_location;
public:
	GoToObjective(const BlockIndex& l) : Objective(Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const override;
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area&, const ActorIndex&) override { }
	void delay(Area&, const ActorIndex&) override { }
	void reset(Area&, const ActorIndex&) override { }
	std::string name() const override { return "go to"; }
	// For testing.
	[[maybe_unused]] BlockIndex getLocation() { return m_location; }
};