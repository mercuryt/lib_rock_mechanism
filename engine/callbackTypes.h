#pragma once
#include "numericTypes/index.h"
#include "numericTypes/types.h"
#include "geometry/cuboid.h"

template<typename Func>
concept DestinationCondition = requires(Func f, const Point3D& b, const Facing4& fa)
{
	{ f(b, fa) } -> std::same_as<std::pair<bool, Point3D>>;
};
template<typename Func>
concept AccessCondition = requires(Func f, const Point3D& b, const Facing4& fa)
{
	{ f(b, fa) } -> std::same_as<bool>;
};
template<typename Func>
concept PointInCuboidCondition = requires(Func f, const Cuboid& cuboid)
{
	{ f(cuboid) } -> std::same_as<std::pair<bool, Point3D>>;
};