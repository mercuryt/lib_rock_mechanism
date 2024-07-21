#pragma once
#include "../objective.h"

class UnequipItemObjective final : public Objective
{
	ItemReference m_item;
	BlockIndex m_block;
public:
	UnequipItemObjective(ItemReference item, BlockIndex block);
	UnequipItemObjective(const Json& data, Area& area);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Unequip; }
	[[nodiscard]] Json toJson() const;

};
