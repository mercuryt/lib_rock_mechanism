#pragma once
#include "../threadedTask.hpp"
#include "../objective.h"
#include "../findsPath.h"

class Actor;
class Item;
class EquipItemObjective final : public Objective
{
	Item& m_item;
public:
	EquipItemObjective(Actor& actor, Item& item);
	EquipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Equip; }
	Json toJson() const;
};
