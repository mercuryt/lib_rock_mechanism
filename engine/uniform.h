#pragma once
#include "config.h"
#include "findsPath.h"
#include "objective.h"
#include "threadedTask.h"
#include "items/itemQuery.h"
#include "threadedTask.hpp"
#include "types.h"
#include <string>
#include <unordered_set>
class UniformObjective;
class Actor;
class Item;
struct UniformElement final 
{
	ItemQuery itemQuery;
	uint32_t quantity;
	UniformElement(const ItemType& itemType, uint32_t quantity = 1, const MaterialType* materialType = nullptr, uint32_t qualityMin = 0);
	[[nodiscard]] bool operator==(const UniformElement& other) const { return &other == this; }
};
struct Uniform final
{
	std::wstring name;
	std::vector<UniformElement> elements;
};
inline void to_json(Json& data, const Uniform* const& uniform){ data = uniform->name; }
class SimulationHasUniformsForFaction final
{
	Faction& m_faction;
	std::unordered_map<std::wstring, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(Faction& faction) : m_faction(faction) { }
	Uniform& createUniform(std::wstring& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	Uniform& at(std::wstring name){ assert(m_data.contains(name)); return m_data.at(name); }
	std::unordered_map<std::wstring, Uniform>& getAll();
};
class SimulationHasUniforms final
{
	std::unordered_map<Faction*, SimulationHasUniformsForFaction> m_data;
public:
	void registerFaction(Faction& faction) { m_data.try_emplace(&faction, faction); }
	void unregisterFaction(Faction& faction) { m_data.erase(&faction); }
	SimulationHasUniformsForFaction& at(Faction& faction);
};
class UniformThreadedTask final : public ThreadedTask
{
	Area& m_area;
	UniformObjective& m_objective;
	FindsPath m_findsPath;
public:
	UniformThreadedTask(UniformObjective& objective);
	void readStep();
	void writeStep();
	void clearReferences();
};
class UniformObjective final : public Objective
{
	HasThreadedTask<UniformThreadedTask> m_threadedTask;
	std::vector<UniformElement> m_elementsCopy;
	ItemIndex m_item;
public:
	UniformObjective(ActorIndex actor);
	UniformObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset(Area& area);
	[[nodiscard]] std::string name() const { return "uniform"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Uniform; }
	// non virtual.
	void equip(ItemIndex item);
	void select(ItemIndex item);
	bool blockContainsItem(BlockIndex block) const { return const_cast<UniformObjective*>(this)->getItemAtBlock(block) != ITEM_INDEX_MAX; }
	ItemIndex getItemAtBlock(BlockIndex block);
	// For testing.
	[[nodiscard]] ItemIndex getItem() { return m_item; }
	friend class UniformThreadedTask;
	
};
