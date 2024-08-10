#pragma once

#include "config.h"
#include "dataVector.h"
#include "items/itemQuery.h"
#include "types.h"

#include <deque>
#include <string>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include <cassert>

class MaterialTypeCategory
{
	static MaterialTypeCategory data;
	DataVector<std::string, MaterialCategoryTypeId> m_name;
public:
	static const MaterialCategoryTypeId byName(const std::string name);
	static void create(std::string name)
	{
		data.m_name.add(name);
	}
};
class SpoilData
{
	static SpoilData data;
	DataVector<MaterialTypeId, SpoilsDataTypeId> m_materialType;
	DataVector<ItemTypeId, SpoilsDataTypeId> m_itemType;
	DataVector<double, SpoilsDataTypeId> m_chance;
	DataVector<Quantity, SpoilsDataTypeId> m_min;
	DataVector<Quantity, SpoilsDataTypeId> m_max;
public:
	static SpoilsDataTypeId create(const MaterialTypeId mt, const ItemTypeId it, const double c, const Quantity mi, const Quantity ma)
	{
		data.m_materialType.add(mt);
		data.m_itemType.add(it);
		data.m_chance.add(c);
		data.m_min.add(mi);
		data.m_max.add(ma);
		return SpoilsDataTypeId::create(data.m_materialType.size() - 1);
	}
	[[nodiscard]] static MaterialTypeId getMaterialType(SpoilsDataTypeId id) { return data.m_materialType[id]; };
	[[nodiscard]] static ItemTypeId getItemType(SpoilsDataTypeId id) { return data.m_itemType[id]; };
	[[nodiscard]] static double getChance(SpoilsDataTypeId id) { return data.m_chance[id]; };
	[[nodiscard]] static Quantity getMin(SpoilsDataTypeId id) { return data.m_min[id]; };
	[[nodiscard]] static Quantity getMax(SpoilsDataTypeId id) { return data.m_max[id]; };
};
struct MaterialTypeConstructionDataParamaters final
{
	std::vector<std::pair<ItemQuery, Quantity>> consumed = {};
	std::vector<std::pair<ItemQuery, Quantity>> unconsumed = {};
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> byproducts = {};
	std::string name;
	SkillTypeId skill;
	Step duration;
};
struct MaterialTypeParamaters final
{
	std::string name;
	Density density;
	uint32_t hardness;
	bool transparent;
	MaterialCategoryTypeId materialTypeCategory = MaterialCategoryTypeId::null();
	std::vector<SpoilsDataTypeId> spoilData = {};
	Temperature meltingPoint = Temperature::null();
	FluidTypeId meltsInto = FluidTypeId::null();
	// Construction.
	std::vector<std::pair<ItemQuery, Quantity>> construction_consumed = {};
	std::vector<std::pair<ItemQuery, Quantity>> construction_unconsumed = {};
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> construction_byproducts = {};
	std::string construction_name = {};
	SkillTypeId construction_skill = SkillTypeId::null();
	Step construction_duration = Step::null();
	// Fire.
	Step burnStageDuration = Step::null(); // How many steps to go from smouldering to burning and from burning to flaming.
	Step flameStageDuration = Step::null(); // How many steps to spend flaming.
	Temperature ignitionTemperature = Temperature::null(); // Temperature at which this material catches fire.
	TemperatureDelta flameTemperature = TemperatureDelta::null(); // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
};
class MaterialType final
{
	static MaterialType data;
	DataVector<std::string, MaterialTypeId> m_name;
	DataVector<Density, MaterialTypeId> m_density;
	DataVector<uint32_t, MaterialTypeId> m_hardness;
	DataBitSet<MaterialTypeId> m_transparent;
	DataVector<MaterialCategoryTypeId, MaterialTypeId> m_materialTypeCategory;
	// Construction.
	DataVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId> m_construction_consumed;
	DataVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId>  m_construction_unconsumed;
	DataVector<std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>>, MaterialTypeId>  m_construction_byproducts;
	DataVector<SkillTypeId, MaterialTypeId> m_construction_skill;
	DataVector<Step, MaterialTypeId> m_construction_duration;
	// How does this material dig?
	// There is no reason why spoils data isn't stored inline in structs rather then in a seperate table.
	DataVector<std::vector<SpoilsDataTypeId>, MaterialTypeId> m_spoilData;
	// What temperatures cause phase changes?
	DataVector<Temperature, MaterialTypeId> m_meltingPoint;
	DataVector<FluidTypeId, MaterialTypeId> m_meltsInto;
	// Fire.
	DataVector<Step, MaterialTypeId> m_burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	DataVector<Step, MaterialTypeId> m_flameStageDuration; // How many steps to spend flaming.
	DataVector<Temperature, MaterialTypeId> m_ignitionTemperature; // Temperature at which this material catches fire.
	DataVector<TemperatureDelta, MaterialTypeId> m_flameTemperature; // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
public:
	static MaterialTypeId create(MaterialTypeParamaters& p);
	[[nodiscard]] static bool empty() { return data.m_density.empty(); }
	[[nodiscard]] static MaterialTypeId byName(std::string name);
	[[nodiscard]] static std::string& getName(MaterialTypeId id) { return data.m_name[id]; };
	[[nodiscard]] static Density getDensity(MaterialTypeId id) { return data.m_density[id]; };
	[[nodiscard]] static uint32_t getHardness(MaterialTypeId id) { return data.m_hardness[id]; };
	[[nodiscard]] static bool getTransparent(MaterialTypeId id) { return data.m_transparent[id]; };
	[[nodiscard]] static MaterialCategoryTypeId getMaterialTypeCategory(MaterialTypeId id) { return data.m_materialTypeCategory[id]; };
	[[nodiscard]] static std::vector<SpoilsDataTypeId>& getSpoilData(MaterialTypeId id) { return data.m_spoilData[id]; };
	[[nodiscard]] static Temperature getMeltingPoint(MaterialTypeId id) { return data.m_meltingPoint[id]; };
	[[nodiscard]] static FluidTypeId getMeltsInto(MaterialTypeId id) { return data.m_meltsInto[id]; };
	// Fire.
	[[nodiscard]] static bool canBurn(MaterialTypeId id) { return data.m_burnStageDuration[id].empty(); }
	[[nodiscard]] static Step getBurnStageDuration(MaterialTypeId id) { return data.m_burnStageDuration[id]; };
	[[nodiscard]] static Step getFlameStageDuration(MaterialTypeId id) { return data.m_flameStageDuration[id]; };
	[[nodiscard]] static Temperature getIgnitionTemperature(MaterialTypeId id) { return data.m_ignitionTemperature[id]; };
	[[nodiscard]] static TemperatureDelta getFlameTemperature(MaterialTypeId id) { return data.m_flameTemperature[id]; };
	// Construct.
	[[nodiscard]] static std::vector<std::pair<ItemQuery, Quantity>> construction_getConsumed(MaterialTypeId id) { return data.m_construction_consumed[id]; };
	[[nodiscard]] static std::vector<std::pair<ItemQuery, Quantity>> construction_getUnconsumed(MaterialTypeId id) { return data.m_construction_unconsumed[id]; };
	[[nodiscard]] static std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> construction_getByproducts(MaterialTypeId id) { return data.m_construction_byproducts[id]; };
	[[nodiscard]] static SkillTypeId construction_getSkill(MaterialTypeId id) { return data.m_construction_skill[id]; };
	[[nodiscard]] static Step construction_getDuration(MaterialTypeId id) { return data.m_construction_duration[id]; };
};
