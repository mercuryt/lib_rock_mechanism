#pragma once
#include "numericTypes/types.h"
#include "geometry/cuboidSet.h"
#include "config/config.h"
#include <vector>
class FluidType;
class Area;
struct DeserializationMemo;
struct FluidSource final
{
	CuboidSet zone;
	FluidTypeId fluidType;
	CollisionVolume level;
	FluidSource(const CuboidSet& b, FluidTypeId ft, CollisionVolume l) : zone(b), fluidType(ft), level(l) { }
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
	void create(const CuboidSet& zone, FluidTypeId fluidType, CollisionVolume level);
	void destroy(const CuboidSet& zone);
	[[nodiscard]] bool contains(const CuboidSet& zone) const;
	[[nodiscard]] const FluidSource& at(const CuboidSet& zone) const;
};
