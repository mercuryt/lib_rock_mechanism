#pragma once

#include "config.h"
#include "dataStructures/strongVector.h"
#include "idTypes.h"
#include "items/itemQuery.h"
#include "types.h"

#include <string>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include <cassert>

class MaterialTypeCategory
{
	StrongVector<std::string, MaterialCategoryTypeId> m_name;
public:
	[[nodiscard]] static MaterialCategoryTypeId size();
	[[nodiscard]] static const MaterialCategoryTypeId byName(const std::string&& name);
	[[nodiscard]] static const std::string& getName(MaterialCategoryTypeId id);
	static void create(std::string name);
};
static MaterialTypeCategory materialTypeCategoryData;
class SpoilData
{
	StrongVector<MaterialTypeId, SpoilsDataTypeId> m_materialType;
	StrongVector<ItemTypeId, SpoilsDataTypeId> m_itemType;
	StrongVector<Percent, SpoilsDataTypeId> m_chance;
	StrongVector<Quantity, SpoilsDataTypeId> m_min;
	StrongVector<Quantity, SpoilsDataTypeId> m_max;
public:
	static SpoilsDataTypeId create(const MaterialTypeId& mt, const ItemTypeId& it, const Percent& c, const Quantity& mi, const Quantity& ma);
	[[nodiscard]] static MaterialTypeId getMaterialType(const SpoilsDataTypeId& id);
	[[nodiscard]] static ItemTypeId getItemType(const SpoilsDataTypeId& id);
	[[nodiscard]] static Percent getChance(const SpoilsDataTypeId& id);
	[[nodiscard]] static Quantity getMin(const SpoilsDataTypeId& id);
	[[nodiscard]] static Quantity getMax(const SpoilsDataTypeId& id);
};
inline SpoilData spoilData;
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
	float valuePerUnitFullDisplacement;
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
	StrongVector<std::string, MaterialTypeId> m_name;
	StrongVector<Density, MaterialTypeId> m_density;
	StrongVector<uint32_t, MaterialTypeId> m_hardness;
	StrongBitSet<MaterialTypeId> m_transparent;
	StrongVector<MaterialCategoryTypeId, MaterialTypeId> m_materialTypeCategory;
	StrongVector<uint, MaterialTypeId> m_valuePerUnitFullDisplacement;
	// Construction.
	StrongVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId> m_construction_consumed;
	StrongVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId>  m_construction_unconsumed;
	StrongVector<std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>>, MaterialTypeId>  m_construction_byproducts;
	StrongVector<SkillTypeId, MaterialTypeId> m_construction_skill;
	StrongVector<Step, MaterialTypeId> m_construction_duration;
	// How does this material dig?
	// There is no reason why spoils data isn't stored inline in structs rather then in a seperate table.
	StrongVector<std::vector<SpoilsDataTypeId>, MaterialTypeId> m_spoilData;
	// What temperatures cause phase changes?
	StrongVector<Temperature, MaterialTypeId> m_meltingPoint;
	StrongVector<FluidTypeId, MaterialTypeId> m_meltsInto;
	// Fire.
	StrongVector<Step, MaterialTypeId> m_burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	StrongVector<Step, MaterialTypeId> m_flameStageDuration; // How many steps to spend flaming.
	StrongVector<Temperature, MaterialTypeId> m_ignitionTemperature; // Temperature at which this material catches fire.
	StrongVector<TemperatureDelta, MaterialTypeId> m_flameTemperature; // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
public:
	static MaterialTypeId create(const MaterialTypeParamaters& p);
	static void setConstructionParamaters(const MaterialTypeId& materialType, const MaterialTypeConstructionDataParamaters& p);
	[[nodiscard]] static bool empty();
	[[nodiscard]] static MaterialTypeId size();
	[[nodiscard]] static MaterialTypeId byName(std::string name);
	[[nodiscard]] static std::string& getName(const MaterialTypeId& id);
	[[nodiscard]] static Density getDensity(const MaterialTypeId& id);
	[[nodiscard]] static uint32_t getHardness(const MaterialTypeId& id);
	[[nodiscard]] static bool getTransparent(const MaterialTypeId& id);
	[[nodiscard]] static uint getValuePerUnitFullDisplacement(const MaterialTypeId& id);
	[[nodiscard]] static MaterialCategoryTypeId getMaterialTypeCategory(const MaterialTypeId& id);
	[[nodiscard]] static std::vector<SpoilsDataTypeId>& getSpoilData(const MaterialTypeId& id);
	[[nodiscard]] static Temperature getMeltingPoint(const MaterialTypeId& id);
	[[nodiscard]] static FluidTypeId getMeltsInto(const MaterialTypeId& id);
	[[nodiscard]] static bool canMelt(const MaterialTypeId& id);
	// Fire.
	[[nodiscard]] static bool canBurn(const MaterialTypeId& id);
	[[nodiscard]] static Step getBurnStageDuration(const MaterialTypeId& id);
	[[nodiscard]] static Step getFlameStageDuration(const MaterialTypeId& id);
	[[nodiscard]] static Temperature getIgnitionTemperature(const MaterialTypeId& id);
	[[nodiscard]] static TemperatureDelta getFlameTemperature(const MaterialTypeId& id);
	// Construct.
	[[nodiscard]] static std::vector<std::pair<ItemQuery, Quantity>>& construction_getConsumed(const MaterialTypeId& id);
	[[nodiscard]] static std::vector<std::pair<ItemQuery, Quantity>>& construction_getUnconsumed(const MaterialTypeId& id);
	[[nodiscard]] static std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>>& construction_getByproducts(const MaterialTypeId& id);
	[[nodiscard]] static SkillTypeId construction_getSkill(const MaterialTypeId& id);
	[[nodiscard]] static Step construction_getDuration(const MaterialTypeId& id);
};
inline MaterialType materialTypeData;
