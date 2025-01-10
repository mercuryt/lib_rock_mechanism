#pragma once
#include "objective.h"
#include "config.h"
#include "input.h"
#include "types.h"
#include "index.h"

struct DeserializationMemo;
class Area;

class GoToObjective final : public Objective
{
	BlockIndex m_location;
public:
	GoToObjective(const BlockIndex& l) : Objective(Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area&, const ActorIndex&) override { }
	void delay(Area&, const ActorIndex&) override { }
	void reset(Area&, const ActorIndex&) override { }
	std::wstring name() const override { return L"go to"; }
	// For testing.
	[[maybe_unused]] BlockIndex getLocation() { return m_location; }
};
class GoToInputAction final : public InputAction
{
public:
	BlockIndex m_block;
	GoToInputAction(ItemIndices actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, const BlockIndex& b);
	void execute();
};