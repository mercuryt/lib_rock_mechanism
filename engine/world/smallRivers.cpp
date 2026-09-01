#include "smallRivers.h"
#include "../fluidType.h"

void SmallRiverData::clear() { flowRate = -1; fluidType.clear(); distanceFromStart = -1; }
bool SmallRiverData::empty() const { return flowRate == 0; }
SmallRiverData::Primitive SmallRiverData::get() const { return {flowRate, fluidType.get(), distanceFromStart}; }
std::string SmallRiverData::toS() const { return "{" + FluidType::getName(fluidType) + ": " + std::to_string(flowRate) + "}"; }
SmallRiverData SmallRiverData::create(Primitive p) { return {{p.flowRate}, {p.fluidType}, p.distanceFromStart}; }
SmallRiverData SmallRiverData::null() { return {0, FluidTypeId::null(), -1}; }