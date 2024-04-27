#pragma once
#include "../threadedTask.hpp"
#include "../objective.h"
#include "../findsPath.h"

class Actor;
class Item;
class Block;

class UnequipItemObjective final : public Objective
{
	Item& m_item;
	Block& m_block;
public:
	UnequipItemObjective(Actor& actor, Item& item, Block& block);
	UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Unequip; }
	Json toJson() const;
};
