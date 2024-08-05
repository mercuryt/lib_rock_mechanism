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
	static MaterialCategoryTypeId create(std::string name)
	{
		data.m_name.add(name);
		return MaterialCategoryTypeId::create(data.m_name.size() - 1);
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
};

struct MaterialTypeParamaters final
{
	DataVector<std::string, MaterialTypeId> m_name;
	DataVector<Density, MaterialTypeId> m_density;
	DataVector<uint32_t, MaterialTypeId> m_hardness;
	DataVector<bool, MaterialTypeId> m_transparent;
	DataVector<MaterialCategoryTypeId, MaterialTypeId> m_materialTypeCategory;
	DataVector<std::vector<SpoilsDataTypeId>, MaterialTypeId> m_spoilData;
	DataVector<Temperature, MaterialTypeId> m_meltingPoint;
	DataVector<FluidTypeId, MaterialTypeId> m_meltsInto;
	DataVector<BurnDataTypeId, MaterialTypeId> m_burnData;
	DataVector<MaterialConstructionDataTypeId, MaterialTypeId> m_constructionData;
	// Construction.
	DataVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialConstructionDataTypeId> m_construction_consumed;
	DataVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialConstructionDataTypeId>  m_construction_unconsumed;
	DataVector<std::vector<std::tuple<const ItemType*, const MaterialType*, Quantity>>, MaterialConstructionDataTypeId>  m_construction_byproducts;
	DataVector<std::string, MaterialConstructionDataTypeId> m_construction_name;
	DataVector<SkillTypeId, MaterialConstructionDataTypeId> m_construction_skill;
	DataVector<Step, MaterialConstructionDataTypeId> m_construction_duration;
	// Fire.
	DataVector<Step, BurnDataTypeId> m_burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	DataVector<Step, BurnDataTypeId> m_flameStageDuration; // How many steps to spend flaming.
	DataVector<Temperature, BurnDataTypeId> m_ignitionTemperature; // Temperature at which this material catches fire.
	DataVector<TemperatureDelta, BurnDataTypeId> m_flameTemperature; // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
};

class MaterialType final
{
	static MaterialType data;
	DataVector<std::string, MaterialTypeId> m_name;
	DataVector<Density, MaterialTypeId> m_density;
	DataVector<uint32_t, MaterialTypeId> m_hardness;
	DataVector<bool, MaterialTypeId> m_transparent;
	DataVector<MaterialCategoryTypeId, MaterialTypeId> m_materialTypeCategory;
	// How does this material dig?
	DataVector<std::vector<SpoilsDataTypeId>, MaterialTypeId> m_spoilData;
	// What temperatures cause phase changes?
	DataVector<Temperature, MaterialTypeId> m_meltingPoint;
	DataVector<FluidTypeId, MaterialTypeId> m_meltsInto;
	// Construction.
	DataVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId> m_construction_consumed;
	DataVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId>  m_construction_unconsumed;
	DataVector<std::vector<std::tuple<const ItemType*, const MaterialType*, Quantity>>, MaterialTypeId>  m_construction_byproducts;
	DataVector<std::string, MaterialTypeId> m_construction_name;
	DataVector<SkillTypeId, MaterialTypeId> m_construction_skill;
	DataVector<Step, MaterialTypeId> m_construction_duration;
	// Fire.
	DataVector<Step, MaterialTypeId> m_burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	DataVector<Step, MaterialTypeId> m_flameStageDuration; // How many steps to spend flaming.
	DataVector<Temperature, MaterialTypeId> m_ignitionTemperature; // Temperature at which this material catches fire.
	DataVector<TemperatureDelta, MaterialTypeId> m_flameTemperature; // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
	static MaterialTypeId create(MaterialTypeParamaters& p);
	// Infastructure.
	[[nodiscard]] static MaterialTypeId byName(const std::string name);
};
