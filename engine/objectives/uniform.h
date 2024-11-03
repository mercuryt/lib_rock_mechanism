#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../uniform.h"
#include "../reference.h"
class Area;
class UniformObjective;

class UniformObjective final : public Objective
{
	std::vector<UniformElement> m_elementsCopy;
	ItemReference m_item;
public:
	UniformObjective(Area& area, const ActorIndex& actor);
	UniformObjective(const Json& data, Area& area, const ActorIndex& actor);
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
	// non virtual.
	void equip(Area& area, const ItemIndex& item, const ActorIndex& actor);
	void select(Area& area, const ItemIndex& item);
	bool blockContainsItem(Area& area, const BlockIndex& block) const { return const_cast<UniformObjective*>(this)->getItemAtBlock(area, block).exists(); }
	ItemIndex getItemAtBlock(Area& area, const BlockIndex& block);
	// For testing.
	[[nodiscard]] ItemIndex getItem() { return m_item.getIndex(); }
	friend class UniformThreadedTask;
	
};
class UniformPathRequest final : public PathRequest
{
	UniformObjective& m_objective;
public:
	UniformPathRequest(Area& area, UniformObjective& objective, const ActorIndex& actor);
	UniformPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult& result);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
};
