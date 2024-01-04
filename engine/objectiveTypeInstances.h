/*
 * Holds instances of objectiveTypes to be referenced by prioritySet.
 */
#pragma once
#include "construct.h"
#include "craft.h"
#include "dig.h"
#include "givePlantsFluid.h"
#include "objective.h"
#include "stockpile.h"
#include "sowSeeds.h"
#include "harvest.h"
#include "givePlantsFluid.h"
#include <memory>
#include <map>
struct ObjectiveTypeInstances
{
	static std::map<std::string, std::unique_ptr<ObjectiveType>> objectiveTypes;
	static std::map<const ObjectiveType*, std::string> objectiveTypeNames;
	inline static void load()
	{
		objectiveTypes["wood working"] = std::make_unique<CraftObjectiveType>(SkillType::byName("wood working"));
		objectiveTypes["metal working"] = std::make_unique<CraftObjectiveType>(SkillType::byName("metal working"));
		objectiveTypes["stone carving"] = std::make_unique<CraftObjectiveType>(SkillType::byName("stone carving"));
		objectiveTypes["bone carving"] = std::make_unique<CraftObjectiveType>(SkillType::byName("bone carving"));
		objectiveTypes["assembling"] = std::make_unique<CraftObjectiveType>(SkillType::byName("assembling"));
		objectiveTypes["dig"] = std::make_unique<DigObjectiveType>();
		objectiveTypes["construct"] = std::make_unique<ConstructObjectiveType>();
		objectiveTypes["stockpile"] = std::make_unique<StockPileObjectiveType>();
		objectiveTypes["sow seeds"] = std::make_unique<SowSeedsObjectiveType>();
		objectiveTypes["give plants fluid"] = std::make_unique<GivePlantsFluidObjectiveType>();
		objectiveTypes["harvest"] = std::make_unique<HarvestObjectiveType>();
		for(auto& [name, objectiveType] : objectiveTypes)
			objectiveTypeNames[objectiveType.get()] = name;
	}
};
