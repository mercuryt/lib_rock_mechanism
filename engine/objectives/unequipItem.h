#pragma once
#include "../objective.h"

class UnequipItemObjective final : public Objective
{
	ItemReference m_item;
	BlockIndex m_block;
public:
	UnequipItemObjective(const ItemReference& item, const BlockIndex& block);
	UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	//TODO: check if block can hold item.
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] Json toJson() const;
};
