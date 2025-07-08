#pragma once
#include "numericTypes/types.h"
#include "geometry/point3D.h"
#include "config.h"
#include <vector>
class FluidType;
class Area;
struct DeserializationMemo;
struct FluidSource final
{
	Point3D point;
	FluidTypeId fluidType;
	CollisionVolume level;
	FluidSource(Point3D b, FluidTypeId ft, CollisionVolume l) : point(b), fluidType(ft), level(l) { }
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
	void create(Point3D point, FluidTypeId fluidType, CollisionVolume level);
	void destroy(Point3D);
	[[nodiscard]] bool contains(Point3D point) const;
	[[nodiscard]] const FluidSource& at(Point3D point) const;
};
