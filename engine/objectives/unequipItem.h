#pragma once
#include "../objective.h"

class UnequipItemObjective final : public Objective
{
	ItemReference m_item;
	Point3D m_point;
public:
	UnequipItemObjective(const ItemReference& item, const Point3D& point);
	UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	//TODO: check if point can hold item.
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]] Json toJson() const;
};
