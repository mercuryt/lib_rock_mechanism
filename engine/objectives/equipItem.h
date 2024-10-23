#pragma once
#include "../objective.h"

class Actor;
class Item;
class EquipItemObjective final : public Objective
{
	ItemReference m_item;
public:
	EquipItemObjective(const ItemReference& item);
	EquipItemObjective(const Json& data, Area& area);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "equip"; }
	Json toJson() const;
};
