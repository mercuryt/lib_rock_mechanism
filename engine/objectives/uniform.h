#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../uniform.h"
class Area;
class UniformObjective;

class UniformPathRequest final : public PathRequest
{
	UniformObjective& m_objective;
public:
	UniformPathRequest(Area& area, UniformObjective& objective);
	void callback(Area& area, FindPathResult& result);
};
class UniformObjective final : public Objective
{
	std::vector<UniformElement> m_elementsCopy;
	ItemIndex m_item;
public:
	UniformObjective(Area& area, ActorIndex actor);
	UniformObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area&);
	void cancel(Area&);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	[[nodiscard]] std::string name() const { return "uniform"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Uniform; }
	// non virtual.
	void equip(Area& area, ItemIndex item);
	void select(ItemIndex item);
	bool blockContainsItem(Area& area, BlockIndex block) const { return const_cast<UniformObjective*>(this)->getItemAtBlock(area, block) != ITEM_INDEX_MAX; }
	ItemIndex getItemAtBlock(Area& area, BlockIndex block);
	// For testing.
	[[nodiscard]] ItemIndex getItem() { return m_item; }
	friend class UniformThreadedTask;
	
};
