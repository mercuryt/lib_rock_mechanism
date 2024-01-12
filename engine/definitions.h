#pragma once

#include "itemQuery.h"

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
	const AttackType loadAttackType(const Json& data, const SkillType& defaultSkill);
	std::pair<ItemQuery, uint32_t> loaditemQuery(const Json& data);
	void loadMaterialTypeConstuctionData();
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
