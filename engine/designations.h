/*
 * BlockIndex designations indicate what action a block is suposed to be involved in.
 */
#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "faction.h"
#include <unordered_set>
#include <unordered_map>
#include <cassert>

enum class BlockDesignation { Dig, Construct, SowSeeds, GivePlantFluid, Harvest, StockPileHaulFrom, StockPileHaulTo, Sleep, Rescue, FluidSource, WoodCutting, BLOCK_DESIGNATION_MAX };
NLOHMANN_JSON_SERIALIZE_ENUM(BlockDesignation, {
		{BlockDesignation::Dig, "Dig"},
		{BlockDesignation::Construct, "Construct"},
		{BlockDesignation::SowSeeds, "SowSeeds"},
		{BlockDesignation::GivePlantFluid, "GivePlantFluid"},
		{BlockDesignation::Harvest, "Harvest"},
		{BlockDesignation::StockPileHaulFrom, "StockPileHaulFrom"},
		{BlockDesignation::StockPileHaulTo, "StockPileHaulTo"},
		{BlockDesignation::Sleep, "Sleep"},
		{BlockDesignation::Rescue, "Rescue"},
		{BlockDesignation::FluidSource, "FluidSource"},
		{BlockDesignation::WoodCutting, "WoodCutting"},
});
