#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../uniform.h"
#include "../reference.h"
class Area;
class UniformObjective;

class UniformPathRequest final : public PathRequest
{
	UniformObjective& m_objective;
public:
	UniformPathRequest(Area& area, UniformObjective& objective);
	UniformPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, FindPathResult& result);
	[[nodiscard]] Json toJson() const;
};
class UniformObjective final : public Objective
{
	std::vector<UniformElement> m_elementsCopy;
	ItemReference m_item;
public:
	UniformObjective(Area& area, ActorIndex actor);
	UniformObjective(const Json& data, Area& area, ActorIndex actor);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "uniform"; }
	// non virtual.
	void equip(Area& area, ItemIndex item, ActorIndex actor);
	void select(Area& area, ItemIndex item);
	bool blockContainsItem(Area& area, BlockIndex block) const { return const_cast<UniformObjective*>(this)->getItemAtBlock(area, block).exists(); }
	ItemIndex getItemAtBlock(Area& area, BlockIndex block);
	// For testing.
	[[nodiscard]] ItemIndex getItem() { return m_item.getIndex(); }
	friend class UniformThreadedTask;
	
};
