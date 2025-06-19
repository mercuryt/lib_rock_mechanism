#pragma once
#include "config.h"
#include "types.h"
#include "vectorContainers.h"
#include <string>
class Items;
struct UniformElement final
{
	ItemTypeId m_itemType;
	MaterialCategoryTypeId m_materialCategoryType;
	MaterialTypeId m_materialType;
	Quantity m_quantity;
	[[nodiscard]] bool operator==(const UniformElement& other) const { return &other == this; }
	[[nodiscard]] bool query(const ItemIndex& item, const Items& items) const;
	static UniformElement create(
		const ItemTypeId& itemType, const Quantity& quantity = Quantity::create(1), const MaterialTypeId& materialType = MaterialTypeId::null(),
		const MaterialCategoryTypeId& materialCageoryType = MaterialCategoryTypeId::null(), const Quality qualityMin = Quality::create(0)
	);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(UniformElement, m_itemType, m_materialCategoryType, m_materialType, m_quantity);
};
struct Uniform final
{
	//TODO: use wide string with WideCharToMultiByte for serialization.
	std::string name;
	std::vector<UniformElement> elements;
	Uniform(std::string n, const std::vector<UniformElement>& e) : name(n), elements(e) { }
	Uniform() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Uniform, name, elements);
};
inline void to_json(Json& data, const Uniform* const& uniform){ data = uniform->name; }
class SimulationHasUniformsForFaction final
{
	FactionId m_faction;
	SmallMap<std::string, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(FactionId faction) : m_faction(faction) { }
	SimulationHasUniformsForFaction() = default;
	Uniform& createUniform(std::string& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	Uniform& byName(std::string name){ assert(m_data.contains(name)); return m_data[name]; }
	SmallMap<std::string, Uniform>& getAll();
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasUniformsForFaction, m_faction, m_data);
};
class SimulationHasUniforms final
{
	SmallMap<FactionId, SimulationHasUniformsForFaction> m_data;
public:
	void registerFaction(const FactionId& faction) { m_data.emplace(faction, faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	SimulationHasUniformsForFaction& getForFaction(const FactionId& faction);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasUniforms, m_data);
};