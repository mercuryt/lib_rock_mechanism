#pragma once

#include "deserilizationMemo.h"
#include "reservable.h"
#include "haul.h"
#include "stockpile.h"
#include "craft.h"
#include "dig.h"
#include "construct.h"
#include "project.h"
#include <memory>

inline std::unique_ptr<DishonorCallback> deserializeDishonorCallback(const Json& data, DeserilizationMemo& deserilizationMemo)
{
	if(data["type"] == "HaulSubprojectDishonorCallback")
	       return std::make_unique<HaulSubprojectDishonorCallback>(data, deserilizationMemo);
	if(data["type"] == "StockPileHasShapeDishonorCallback")
		return std::make_unique<StockPileHasShapeDishonorCallback>(data, deserilizationMemo);
	if(data["type"] == "ProjectRequiredShapeDishonorCallback")
		return std::make_unique<ProjectRequiredShapeDishonoredCallback>(data, deserilizationMemo);
	if(data["type"] == "CraftStepProjectHasShapeDishonorCallback")
		return std::make_unique<CraftStepProjectHasShapeDishonorCallback>(data, deserilizationMemo);
	if(data["type"] == "ConstructionLocationDishonorCallback")
		return std::make_unique<ConstructionLocationDishonorCallback>(data, deserilizationMemo);
	assert(data["type"] == "DigLocationDishonorCallback");
	return std::make_unique<DigLocationDishonorCallback>(data, deserilizationMemo);
}
