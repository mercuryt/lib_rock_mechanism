#pragma once
#include "config.h"
#include "item.h"
#include "objective.h"
#include "threadedTask.h"
#include <string>
#include <unordered_set>
class EquipItemObjective;
class UniformObjective;
class Actor;
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
	const Faction& m_faction;
	std::unordered_map<std::wstring, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(const Faction& faction) : m_faction(faction) { }
	Uniform& createUniform(std::wstring& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	Uniform& at(std::wstring name){ assert(m_data.contains(name)); return m_data.at(name); }
	std::unordered_map<std::wstring, Uniform>& getAll();
};
class SimulationHasUniforms final
{
	std::unordered_map<const Faction*, SimulationHasUniformsForFaction> m_data;
public:
	void registerFaction(const Faction& faction) { m_data.try_emplace(&faction, faction); }
	void unregisterFaction(const Faction& faction) { m_data.erase(&faction); }
	SimulationHasUniformsForFaction& at(const Faction& faction);
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
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Equip; }
	Json toJson() const;
	// non virtual methods.
	bool blockContainsItem(const Block& block) const { return const_cast<EquipItemObjective*>(this)->getItemAtBlock(block) != nullptr; }
	Item* getItemAtBlock(const Block& block);
	void select(Item& item);
	friend class EquipItemThreadedTask;
};
class UniformThreadedTask final : public ThreadedTask
{
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
	Item* m_item;
public:
	UniformObjective(Actor& actor);
	UniformObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	[[nodiscard]] std::string name() const { return "uniform"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Uniform; }
	// non virtual.
	void equip(Item& item);
	void select(Item& item);
	bool blockContainsItem(const Block& block) const { return const_cast<UniformObjective*>(this)->getItemAtBlock(block) != nullptr; }
	Item* getItemAtBlock(const Block& block);
	// For testing.
	[[nodiscard]] Item* getItem() { return m_item; }
	friend class UniformThreadedTask;
	
};
class ActorHasUniform final
{
	Actor& m_actor;
	Uniform* m_uniform;
	UniformObjective* m_objective;
public:
	ActorHasUniform(Actor& actor) : m_actor(actor), m_uniform(nullptr), m_objective(nullptr) { }
	void set(Uniform& uniform);
	void unset();
	void recordObjective(UniformObjective& objective);
	void clearObjective(UniformObjective& objective);
	bool exists() const { return m_uniform; }
	Uniform& get() { return *m_uniform; }
	const Uniform& get() const { return *m_uniform; }
};
