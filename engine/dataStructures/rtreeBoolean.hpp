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
		const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
		const auto leafCount = node.getLeafCount();
		auto begin = interceptMask.begin();
		auto end = begin + nodeSize;
		auto leafEnd = begin + leafCount;
		if(leafCount != 0 && interceptMask.head(leafCount).any())
		{
			for(auto iter = begin; iter != leafEnd; ++iter)
				if(*iter)
				{
					Cuboid cuboid = nodeCuboids[iter - begin];
					if(condition(cuboid))
						return cuboid;
				}
		}
		// TODO: Would it be better to check the whole intercept mask and continue if all are empty before checking leafs?
		const auto childCount = node.getChildCount();
		if(childCount == 0 || !interceptMask.tail(childCount).any())
			continue;
		const auto& nodeChildren = node.getChildIndices();
		const auto offsetOfFirstChild = node.offsetOfFirstChild();
		if(offsetOfFirstChild == 0)
		{
			// Unrollable version for nodes where every slot is a child.
			for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
				if(*iter)
					openList.insert(nodeChildren[iter - begin]);
		}
		else
		{
			for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
				if(*iter)
					openList.insert(nodeChildren[iter - begin]);
		}
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