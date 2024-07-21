#pragma once

#include "config.h"
#include "items/itemQuery.h"
#include "types.h"

#include <deque>
#include <string>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include <cassert>

struct MaterialType;
struct ItemType;
struct FluidType;
struct SkillType;

struct MaterialTypeCategory
{
	const std::string name;
	// Infastructure.
	[[nodiscard]] bool operator==(const MaterialTypeCategory& materialTypeCategory) const { return this == &materialTypeCategory; }
	static const MaterialTypeCategory& byName(const std::string name);
};
inline std::vector<MaterialTypeCategory> materialTypeCategoryDataStore;
inline void to_json(Json& data, const MaterialTypeCategory* const & materialTypeCategory) { data = materialTypeCategory->name; }
inline void from_json(const Json& data, const MaterialTypeCategory*& materialTypeCategory) { materialTypeCategory = &MaterialTypeCategory::byName(data.get<std::string>()); }

struct SpoilData
{
	const MaterialType& materialType;
	const ItemType& itemType;
	const double chance = 0;
	const uint32_t min = 0;
	const uint32_t max = 0;
	SpoilData( const MaterialType& mt, const ItemType& it, const double c, const uint32_t mi, const uint32_t ma) : 
	materialType(mt), itemType(it), chance(c), min(mi), max(ma) { }
};

struct BurnData
{
	const Step burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	const Step flameStageDuration; // How many steps to spend flaming.
	const Temperature ignitionTemperature; // Temperature at which this material catches fire.
	const Temperature flameTemperature; // Temperature given off by flames from this material. The temperature given off by burn stage is a fraction of flame stage based on a config setting.
	BurnData(const Step bsd, const Step fsd, const Temperature it, const Temperature ft) : 
	burnStageDuration(bsd), flameStageDuration(fsd), ignitionTemperature(it), flameTemperature(ft) { }
};
inline std::deque<BurnData> burnDataStore;

struct MaterialConstructionData
{
	std::vector<std::pair<ItemQuery, uint32_t>> consumed;
	std::vector<std::pair<ItemQuery, uint32_t>> unconsumed;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> byproducts;
	std::string name;
	const SkillType& skill;
	Step duration;
	MaterialConstructionData( std::string n, const SkillType& s, Step d) : name(n), skill(s), duration(d) { }
	// Infastructure.
	[[nodiscard]] bool operator==(const MaterialConstructionData& materialConstructionData) const { return this == &materialConstructionData; }
	[[nodiscard]] static MaterialConstructionData& byNameSpecialized(const std::string name, const MaterialType& materialType);
};
inline std::deque<MaterialConstructionData> materialConstructionDataStore;
inline std::deque<MaterialConstructionData> materialConstructionSpecializedDataStore;

struct MaterialType
{
	// How does this material dig?
	std::vector<SpoilData> spoilData;
	const std::string name;
	// What temperatures cause phase changes?
	Temperature meltingPoint = 0;
	const FluidType* meltsInto = nullptr;
	BurnData* burnData = nullptr;
	MaterialConstructionData* constructionData = nullptr;
	const MaterialTypeCategory* materialTypeCategory = nullptr;
	const Density density = 0;
	const uint32_t hardness = 0;
	const bool transparent = false;
	MaterialType(std::string n, Density d, uint32_t h, bool t) : name(n), density(d), hardness(h), transparent(t) { }
	// Infastructure.
	[[nodiscard]] bool operator==(const MaterialType& materialType) const { return this == &materialType; }
	[[nodiscard]] static const MaterialType& byName(const std::string name);
	[[nodiscard]] static MaterialType& byNameNonConst(const std::string name);
};
inline std::vector<MaterialType> materialTypeDataStore;
inline void to_json(Json& data, const MaterialType* const& materialType)
{
	data = materialType == nullptr ? "0" : materialType->name;
}
inline void to_json(Json& data, const MaterialType& materialType){ data = materialType.name; }
inline void from_json(const Json& data, const MaterialType*& materialType)
{
	std::string name = data.get<std::string>();
	materialType = name == "0" ? nullptr : &MaterialType::byName(name);
}
