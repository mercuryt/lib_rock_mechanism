#pragma once

#include "config.h"
#include "itemQuery.h"
#include "types.h"

#include <deque>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

struct MaterialType;
struct ItemType;
class ItemQuery;
struct FluidType;
struct SkillType;

struct MaterialTypeCategory
{
	const std::string name;
	// Infastructure.
	bool operator==(const MaterialTypeCategory& materialTypeCategory) const { return this == &materialTypeCategory; }
	static const MaterialTypeCategory& byName(const std::string name);
};
inline std::vector<MaterialTypeCategory> materialTypeCategoryDataStore;

struct SpoilData
{
	const MaterialType& materialType;
	const ItemType& itemType;
	const double chance;
	const uint32_t min;
	const uint32_t max;
};

struct BurnData
{
	const Temperature ignitionTemperature; // Temperature at which this material catches fire.
	const Temperature flameTemperature; // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
	const Step burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	const Step flameStageDuration; // How many steps to spend flaming.
	// Infastructure.
};
inline std::deque<BurnData> burnDataStore;

struct MaterialConstructionData
{
	std::string name;
	const SkillType& skill;
	Step duration;
	std::vector<std::pair<ItemQuery, uint32_t>> consumed;
	std::vector<std::pair<ItemQuery, uint32_t>> unconsumed;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> byproducts;
	// Infastructure.
	bool operator==(const MaterialConstructionData& materialConstructionData) const { return this == &materialConstructionData; }
	static MaterialConstructionData& byNameSpecialized(const std::string name, const MaterialType& materialType);
};
inline std::deque<MaterialConstructionData> materialConstructionDataStore;
inline std::deque<MaterialConstructionData> materialConstructionSpecializedDataStore;

struct MaterialType
{
	const std::string name;
	const MaterialTypeCategory* materialTypeCategory;
	const Density density;
	const uint32_t hardness;
	const bool transparent;
	// What temperatures cause phase changes?
	const Temperature meltingPoint;
	const FluidType* meltsInto;
	// How does this material dig?
	std::vector<SpoilData> spoilData;
	BurnData* burnData;
	MaterialConstructionData* constructionData;
	// Infastructure.
	bool operator==(const MaterialType& materialType) const { return this == &materialType; }
	static const MaterialType& byName(const std::string name);
	static MaterialType& byNameNonConst(const std::string name);
};
inline std::vector<MaterialType> materialTypeDataStore;
inline void to_json(Json& data, const MaterialType* const& materialType){ data = materialType->name; }
inline void to_json(Json& data, const MaterialType& materialType){ data = materialType.name; }
inline void from_json(const Json& data, const MaterialType*& materialType){ materialType = &MaterialType::byName(data.get<std::string>()); }
