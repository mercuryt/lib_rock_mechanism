#pragma once
#include "numericTypes/types.h"
#include "config.h"
#include <vector>
class FluidType;
class Area;
struct DeserializationMemo;
struct FluidSource final
{
	BlockIndex block;
	FluidTypeId fluidType;
	CollisionVolume level;
	FluidSource(BlockIndex b, FluidTypeId ft, CollisionVolume l) : block(b), fluidType(ft), level(l) { }
	FluidSource(const Json& data, DeserializationMemo& deserializationMemo);
};
class AreaHasFluidSources final
{
	Area& m_area;
	std::vector<FluidSource> m_data;
public:
	AreaHasFluidSources(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void doStep();
	void create(BlockIndex block, FluidTypeId fluidType, CollisionVolume level);
	void destroy(BlockIndex);
	[[nodiscard]] bool contains(BlockIndex block) const;
	[[nodiscard]] const FluidSource& at(BlockIndex block) const;
};
