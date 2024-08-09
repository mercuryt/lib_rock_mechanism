#pragma once

#include "items/itemQuery.h"
#include "materialType.h"

#include <filesystem>
#include <string>

struct AttackType;
struct SkillType;

using Json = nlohmann::json;
namespace definitions
{
	inline std::filesystem::path path = "data";
	Json tryParse(std::string filePath);
	void loadShapes();
	void loadFluidTypes();
	void loadMoveTypes();
	void loadMaterialTypes();
	void loadSkillTypes();
	AttackTypeId loadAttackType(const Json& data, SkillTypeId defaultSkill);
	std::pair<ItemQuery, Quantity> loadItemQuery(const Json& data);
	std::unordered_map<std::string, MaterialTypeConstructionDataParamaters> loadMaterialTypeConstuctionData();
	//void loadMedicalProjectTypes()
	void loadCraftJobs();
	void loadWeaponsData();
	void loadItemTypes();
	void loadPlantSpecies();
	void loadBodyPartTypes();
	void loadBodyTypes();
	void loadAnimalSpecies();
	void load();
}
