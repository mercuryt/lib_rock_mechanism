#include "cuboidSet.hpp"
#include "cuboid.h"
#include "offsetCuboid.h"

template struct CuboidSetBase<Cuboid, Point3D, CuboidSet>;
template struct CuboidSetBase<OffsetCuboid, Offset3D, OffsetCuboidSet>;

template struct CuboidSetConstIteratorBase<Cuboid, Point3D, CuboidSet>;
template struct CuboidSetConstIteratorBase<OffsetCuboid, Offset3D, OffsetCuboidSet>;

CuboidSet CuboidSet::create(const Cuboid& cuboid)
{
	CuboidSet output;
	output.add(cuboid);
	return output;
}
CuboidSet CuboidSet::create([[maybe_unused]]const OffsetCuboid& spaceBoundry, const Point3D& pivot, const Facing4& newFacing, const OffsetCuboidSet& cuboids)
{
	CuboidSet output;
	for(OffsetCuboid offset : cuboids)
	{

		offset.rotate2D(newFacing);
		offset = offset.relativeToPoint(pivot);
		assert(spaceBoundry.contains(offset));
		output.add(Cuboid::create(offset));
	}
	return output;
}
void to_json(Json& data, const CuboidSet& cuboidSet)
{
	for(const Cuboid& cuboid : cuboidSet.getCuboids())
		data.push_back({cuboid.m_high, cuboid.m_low});
}
void from_json(const Json& data, CuboidSet& cuboidSet)
{
	for(const Json& pair : data)
	{
		Cuboid cuboid(pair[0].get<Point3D>(), pair[1].get<Point3D>());
		cuboidSet.add(cuboid);
	}
}
void to_json(Json& data, const OffsetCuboidSet& cuboidSet)
{
	for(const OffsetCuboid& cuboid : cuboidSet.getCuboids())
		data.push_back(std::pair{cuboid.m_high, cuboid.m_low});
}
void from_json(const Json& data, OffsetCuboidSet& cuboidSet)
{
	for(const Json& pair : data)
	{
		OffsetCuboid cuboid(pair[0].get<Offset3D>(), pair[1].get<Offset3D>());
		cuboidSet.add(cuboid);
	}
}

