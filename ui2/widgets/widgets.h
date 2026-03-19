#pragma once
#include<functional>
#include "../../engine/numericTypes/idTypes.h"
#include "../../engine/numericTypes/types.h"
struct MaterialTypeId;
struct FluidTypeId;
class PlantSpeciesId;
enum class PointFeatureTypeId;

// Return bool without [[nodiscard]] to mimic ImGui::Input* calls.
namespace widgets
{
	bool materialType(MaterialTypeId* result, std::function<bool(const MaterialTypeId)>&& predicate = {});
	bool materialCategoryType(MaterialCategoryTypeId* result, std::function<bool(const MaterialCategoryTypeId)>&& predicate = {});
	bool featureType(PointFeatureTypeId* result, std::function<bool(const PointFeatureTypeId)>&& predicate = {});
	bool fluidType(FluidTypeId* result, std::function<bool(const FluidTypeId)>&& predicate = {});
	bool plantSpecies(PlantSpeciesId* result, std::function<bool(const PlantSpeciesId)>&& condition = {});
	bool animalSpecies(AnimalSpeciesId* result, std::function<bool(const AnimalSpeciesId)>&& condition = {});
	bool itemType(ItemTypeId* result, std::function<bool(const ItemTypeId)>&& condition = {});
	bool faction(FactionId* result, const Simulation& simulation, std::function<bool(const FactionId)>&& condition = {});
	bool facing(Facing4* result);
	bool itemTypeAndMaterialTypeOrCategory(ItemTypeId* itemType, MaterialTypeId* materialType, MaterialCategoryTypeId* categoryType, std::function<bool(const ItemTypeId)>&& condition = {});
	bool craftJobType(CraftJobTypeId* result);
}