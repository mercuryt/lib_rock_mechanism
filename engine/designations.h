/*
 * Point3D designations indicate what action a point is suposed to be involved in.
 */
#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "faction.h"
#include <cassert>

enum class SpaceDesignation { Dig, Construct, SowSeeds, GivePlantFluid, Harvest, StockPileHaulFrom, StockPileHaulTo, Sleep, Rescue, FluidSource, WoodCutting, SPACE_DESIGNATION_MAX };
NLOHMANN_JSON_SERIALIZE_ENUM(SpaceDesignation, {
		{SpaceDesignation::Dig, "Dig"},
		{SpaceDesignation::Construct, "Construct"},
		{SpaceDesignation::SowSeeds, "SowSeeds"},
		{SpaceDesignation::GivePlantFluid, "GivePlantFluid"},
		{SpaceDesignation::Harvest, "Harvest"},
		{SpaceDesignation::StockPileHaulFrom, "StockPileHaulFrom"},
		{SpaceDesignation::StockPileHaulTo, "StockPileHaulTo"},
		{SpaceDesignation::Sleep, "Sleep"},
		{SpaceDesignation::Rescue, "Rescue"},
		{SpaceDesignation::FluidSource, "FluidSource"},
		{SpaceDesignation::WoodCutting, "WoodCutting"},
});
