#pragma once
#include "faction.h"
#include "objective.h"
#include "reservable.h"
#include "haul.h"
#include "area/stockpile.h"
#include "craft.h"
#include "projects/dig.h"
#include "projects/construct.h"
#include "project.h"
#include <memory>

struct DeserializationMemo;

inline std::unique_ptr<DishonorCallback> deserializeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	if(data["type"] == "HaulSubprojectDishonorCallback")
	       return std::make_unique<HaulSubprojectDishonorCallback>(data, deserializationMemo);
	if(data["type"] == "StockPileHasShapeDishonorCallback")
		return std::make_unique<StockPileHasShapeDishonorCallback>(data, deserializationMemo);
	if(data["type"] == "ProjectRequiredShapeDishonorCallback")
		return std::make_unique<ProjectRequiredShapeDishonoredCallback>(data, deserializationMemo);
	if(data["type"] == "CraftStepProjectHasShapeDishonorCallback")
		return std::make_unique<CraftStepProjectHasShapeDishonorCallback>(data, deserializationMemo);
	if(data["type"] == "ConstructionLocationDishonorCallback")
		return std::make_unique<ConstructionLocationDishonorCallback>(data, deserializationMemo);
	if(data["type"] == "CannotCompleteObjectiveDishonorCallback")
		return std::make_unique<CannotCompleteObjectiveDishonorCallback>(area, data);
	assert(data["type"] == "DigLocationDishonorCallback");
	return std::make_unique<DigLocationDishonorCallback>(data, deserializationMemo);
}
