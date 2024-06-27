#pragma once
#include "../threadedTask.hpp"
#include "../objective.h"
#include "../findsPath.h"

class Actor;
class Item;
class EquipItemObjective final : public Objective
{
	ItemIndex m_item;
public:
	EquipItemObjective(ActorIndex actor, ItemIndex item);
	EquipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Equip; }
	Json toJson() const;
};
