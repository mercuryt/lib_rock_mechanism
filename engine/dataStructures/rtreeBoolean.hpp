#pragma once
#include "rtreeBoolean.h"
template<typename ShapeT>
Cuboid RTreeBoolean::queryGetLeafWithCondition(const ShapeT& shape, auto&& condition) const
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(RTreeNodeIndex::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		BitSet interceptMask = BitSet64::create(nodeCuboids.indicesOfIntersectingCuboids(shape));
		BitSet leafInterceptMask = interceptMask;
		const auto leafCount = node.getLeafCount();
		leafInterceptMask.clearAllAfterInclusive(leafCount);
		while(!leafInterceptMask.empty())
		{
			const auto arrayIndex = leafInterceptMask.getNextAndClear();
			Cuboid cuboid = nodeCuboids[arrayIndex];
			if(condition(cuboid))
				return cuboid;
		}
		interceptMask.clearAllBefore(leafCount);
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