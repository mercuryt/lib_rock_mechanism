#pragma once
#include "../objective.h"

class Actor;
class Item;
class EquipItemObjective final : public Objective
{
	ItemReference m_item;
public:
	EquipItemObjective(ItemReference item);
	EquipItemObjective(const Json& data, Area& area);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Equip; }
	Json toJson() const;
};
