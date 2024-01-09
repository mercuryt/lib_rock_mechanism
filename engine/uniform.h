#pragma once
#include "config.h"
#include "item.h"
#include "objective.h"
#include "threadedTask.h"
#include <string>
#include <unordered_set>
class EquipItemObjective;
struct UniformElement final 
{
	ItemQuery itemQuery;
	uint32_t quantity;
	UniformElement(const ItemType& itemType, const MaterialType* materialType, uint32_t quality = 0, uint32_t quantity = 0);
};
struct Uniform final
{
	std::wstring name;
	std::vector<UniformElement> elements;
};
class SimulationHasUniformsForFaction final
{
	const Faction& m_faction;
	std::unordered_map<std::wstring, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(const Faction& faction) : m_faction(faction) { }
	Uniform& createUniform(std::wstring& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	std::unordered_map<std::wstring, Uniform>& getAll();
};
class SimulationHasUniforms final
{
	std::unordered_map<const Faction*, SimulationHasUniformsForFaction> m_data;
public:
	void registerFaction(const Faction& faction) { m_data.try_emplace(&faction, faction); }
	void unregisterFaction(const Faction& faction) { m_data.erase(&faction); }
	SimulationHasUniformsForFaction& at(const Faction& faction) { return m_data.at(&faction); }
};
class EquipItemThreadedTask final : public ThreadedTask
{
	EquipItemObjective& m_objective;
	FindsPath m_findsPath;
public:
	EquipItemThreadedTask(EquipItemObjective& objective);
	void readStep();
	void writeStep();
	void clearReferences();
};
class EquipItemObjective final : public Objective
{
	ItemQuery m_itemQuery;
	Item* m_item;
	HasThreadedTask<EquipItemThreadedTask> m_threadedTask;
public:
	EquipItemObjective(Actor& actor, ItemQuery& itemQuery);
	EquipItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	[[nodiscard]] std::string name() const { return "equip"; }
	[[nodiscard]]ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Equip; }
	Json toJson() const;
	// non virtual methods.
	bool blockContainsItem(const Block& block) const { return const_cast<EquipItemObjective*>(this)->getItemAtBlock(block) != nullptr; }
	Item* getItemAtBlock(const Block& block);
	void select(Item& item);
	friend class EquipItemThreadedTask;
};
