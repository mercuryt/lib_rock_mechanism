#pragma once
#include "../threadedTask.hpp"
#include "../objective.h"
#include "../findsPath.h"

class Actor;
class Item;

class UnequipItemObjective final : public Objective
{
	Item& m_item;
	BlockIndex m_block;
public:
	UnequipItemObjective(Actor& actor, Item& item, BlockIndex block);
	UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Unequip; }
	[[nodiscard]] Json toJson() const;
};
