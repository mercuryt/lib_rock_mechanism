#pragma once
#include "../objective.h"

class UnequipItemObjective final : public Objective
{
	ItemIndex m_item;
	BlockIndex m_block;
public:
	UnequipItemObjective(ActorIndex actor, ItemIndex item, BlockIndex block);
	UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area&);
	void cancel(Area&);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Unequip; }
	[[nodiscard]] Json toJson() const;
};
