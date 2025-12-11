#include "rtreeBoolean.h"
template<typename ShapeT>
Cuboid RTreeBoolean::queryGetLeafWithCondition(const ShapeT& shape, auto&& condition) const
{
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		BitSet interceptMask = BitSet64::create(nodeCuboids.indicesOfIntersectingCuboids(shape));
		const auto leafCount = node.getLeafCount();
		while(!interceptMask.empty())
		{
			const auto arrayIndex = interceptMask.getNextAndClear();
			if(arrayIndex >= leafCount)
				break;
			Cuboid cuboid = nodeCuboids[arrayIndex];
			if(condition(cuboid))
				return cuboid;
		}
		if(!interceptMask.empty())
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
	}
	return {};
}
template<typename ShapeT>
Point3D RTreeBoolean::queryGetPointWithCondition(const ShapeT& shape, auto&& condition) const
{
	const Cuboid found = queryGetLeafWithCondition(shape, condition);
	if(found.empty())
		return Point3D::null();
	return found.intersectionPoint(shape);
}
void RTreeBoolean::queryRemove(CuboidSet& set) const
{
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		BitSet interceptMask = BitSet64::create(nodeCuboids.indicesOfIntersectingCuboids(set));
		const auto leafCount = node.getLeafCount();
		while(!interceptMask.empty())
		{
			const auto arrayIndex = interceptMask.getNextAndClear();
			if(arrayIndex >= leafCount)
				break;
			Cuboid cuboid = nodeCuboids[arrayIndex];
			set.remove(cuboid);
		}
		if(!interceptMask.empty())
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
	}
}