#include "smallRivers.h"
#include "../fluidType.h"

void SmallRiverData::clear() { flowRate = -1; fluidType.clear(); distanceFromStart.clear(); connections.clear(); }
bool SmallRiverData::empty() const { return flowRate == -1; }
SmallRiverData::Primitive SmallRiverData::get() const { return {flowRate, fluidType.get(), distanceFromStart.get(), connections.get()}; }
std::string SmallRiverData::toS() const { return "{" + FluidType::getName(fluidType) + ": " + std::to_string(flowRate) + "}"; }
SmallRiverData SmallRiverData::create(Primitive p) { return {{p.flowRate}, {p.fluidType}, {p.distanceFromStart}, {p.connections}}; }
SmallRiverData SmallRiverData::null() { return {-1, FluidTypeId::null(), Distance::null(), BitSet<uint8_t, 8u>(0u)}; }