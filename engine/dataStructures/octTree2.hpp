#include "octTree2.h"
template<typename NodeData, int splitThreashold, int mergeThreadshold>
void Bool3D<NodeData, splitThreashold, mergeThreadshold>::split(Node& node, const Cuboid& cuboid)
{
	auto splitData = node.data.splitIntoOctants();
	node.childData = Index::create(m_nodes.size());
	EightNodes& nodeSet = m_nodes.add();
	for(Octant octant = 0; octant < 8; ++octant)
		nodeSet.nodes[octant] = { spiltData[octant] };
	nodeSet.cuboids = cuboid.splitIntoOctants();
	nodeSet.center = cuboid.getCenter();
	m_parents.add(&node);
}

template<typename NodeData, int splitThreashold, int mergeThreadshold>
void Bool3D<NodeData, splitThreashold, mergeThreadshold>::merge(Node& node)
{
	if(m_nodes.size() != node.childData.get() + 1)
	{
		m_nodes.erase(node.childData);
		m_parents.erase(node.childData);
		// Update stored index in the node which previously pointed to the end of m_nodes and now points where the destroyed node used to be.
		m_parents[node.childData]->childData = node.childData;
	}
	else
	{
		m_nodes.popBack();
		m_parents.popBack();
	}
	node.childData.clear();
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
void Bool3D<NodeData, splitThreashold, mergeThreadshold>::maybeSetPoint(const Point3D& coordinates)
{
	// There is no single top level node, instead we start with the first set of eight.
	Octant octant = getOctant(m_center, coordinates);
	EightNodes* nodeSet = &m_nodes[Index::create(0)];
	while(true)
	{
		Node& node = nodeSet->nodes[octant];
		if(node.childData.empty())
		{
			// Only insert at the lowest level.
			node.contents.maybeInsert(coordinates);
			if(node.data.size() > splitThreashold)
				split(node, nodeSet.cuboids[octant]);
			else if(node.data.size() < mergeThreashold)
				merge(node);
			break;
		}
		// Select new node at next level down.
		nodeSet = &m_nodes[node.childData];
		octant = getOctant(nodeSet.center, coordinates);
	}
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
void Bool3D<NodeData, splitThreashold, mergeThreadshold>::maybeUnsetPoint(const Point3D& coordinates)
{
	// There is no single top level node, instead we start with the first set of eight.
	Octant octant = getOctant(m_center, coordinates);
	EightNodes* nodeSet = &m_nodes[Index::create(0)];
	while(true)
	{
		Node& node = nodeSet->nodes[octant];
		if(node.childData.empty())
		{
			// Only insert at the lowest level.
			node.contents.maybeRemove(coordinates);
			if(node.data.size() > splitThreashold)
				split(node, nodeSet.cuboids[octant]);
			else if(node.data.size() < mergeThreashold)
				merge(node);
			break;
		}
		// Select new node at next level down.
		nodeSet = &m_nodes[node.childData];
		octant = getOctant(nodeSet.center, coordinates);
	}
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
void Bool3D<NodeData, splitThreashold, mergeThreadshold>::maybeSetCuboid(const Cuboid& cuboid)
{
	SmallSet<Index> openList;
	// There is no single top level node, instead we start with the first set of eight.
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		NodeSet& nodeSet = m_nodes[openList.back()];
		openList.popBack();
		const auto interceptMask = nodeSet.cuboids.indicesOfIntersectingCuboids(cuboid);
		for(Octant octant = 0; octant < 8; ++octant)
			if(interceptMask[octant])
			{
				Node& node = nodeSet.nodes[octant];
				const Index& childData = node.childData;
				if(childData.exists())
					openList.pushBack(childData);
				else
				{
					node.data.maybeInsert(cuboid.getIntersectionWith(nodeSet.cuboids[octant]));
					if(node.data.size() > splitThreashold)
						split(node, nodeSet.cuboids[octant]);
					else if(node.data.size() < mergeThreashold)
						merge(node);
				}
			}
	}
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
void Bool3D<NodeData, splitThreashold, mergeThreadshold>::maybeUnsetCuboid(const Cuboid& cuboid)
{
	SmallSet<Index> openList;
	SmallSet<std::pair<Index, Octant>> toMerge;
	// There is no single top level node, instead we start with the first set of eight.
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		const Index index = openList.back();
		NodeSet& nodeSet = m_nodes[index];
		openList.popBack();
		const auto interceptMask = nodeSet.cuboids.indicesOfIntersectingCuboids(cuboid);
		for(Octant octant = 0; octant < 8; ++octant)
			if(interceptMask[octant])
			{
				Node& node = nodeSet.nodes[octant];
				const Index& childData = node.childData;
				if(childData.exists())
					openList.pushBack(childData);
				else
				{
					node.data.maybeRemove(cuboid.getIntersectionWith(nodeSet.cuboids[octant]));
					if(node.data.size() > splitThreashold)
						toMerge.emplace(childData, octant);
						split(node, nodeSet.cuboids[octant]);
					else if(node.data.size() < mergeThreashold)
						toDestory.emplaceBack(index, octant);
				}
			}
	}
	SmallSet<Index> toDestroy;
	for(const auto& [index, octant] : toMerge)
	{
		const auto& node = m_nodes[index].nodes[octant];
		if(m_nodes.size() != node.childData.get() + 1)
		{
			// This is not the last node.
			m_nodes.erase(node.childData);
			m_parents.erase(node.childData);
			// Update stored index in the node which previously pointed to the end of m_nodes and now points where the destroyed node used to be.
			const Index overwritten = m_parents[node.childData]->childData;
			m_parents[node.childData]->childData = node.childData;
			// Update toMerge, something we haven't iterated yet may have become invalid with the data move.
			toMerge.maybeUpdate(overwritten, node.childData);
		}
		else
		{
			m_nodes.popBack();
			m_parents.popBack();
		}
		node.childData.clear();
	}

	merge(m_nodes[index].nodes[octant]);
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
bool Bool3D<NodeData, splitThreashold, mergeThreadshold>::queryPoint(const Point3D& point)
{
	// There is no single top level node, instead we start with the first set of eight.
	Octant octant = getOctant(m_center, coordinates);
	EightNodes* nodeSet = &m_nodes[Index::create(0)];
	while(true)
	{
		Node& node = nodeSet->nodes[octant];
		if(node.childData.empty())
			return node.data.contains(point);
		// Select new node at next level down.
		nodeSet = &m_nodes[node.childData];
		octant = getOctant(nodeSet.center, coordinates);
	}
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
bool Bool3D<NodeData, splitThreashold, mergeThreadshold>::queryCuboidAny(const Cuboid& cuboid)
{
	SmallSet<Index> openList;
	// There is no single top level node, instead we start with the first set of eight.
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		NodeSet& nodeSet = m_nodes[openList.back()];
		openList.popBack();
		const auto containedMask = nodeSet.cuboids.indicesOfContainedCuboids(cuboid);
		for(Octant octant = 0; octant < 8; ++octant)
			if(containedMask[octant] && nodeSet.nodes[octant].data.intersects(cuboid))
				return true;
		const auto interceptMask = nodeSet.cuboids.indicesOfIntersectingCuboids(cuboid);
		interceptMask -= containedMask
		for(Octant octant = 0; octant < 8; ++octant)
			if(interceptMask[octant])
			{
				Node& node = nodeSet.nodes[octant];
				const OctTreeIndex& childData = node.childData;
				if(childData.exists())
					openList.pushBack(childData);
				else if(nodeSet.nodes[octant].data.intersects(cuboid))
					return true;
			}
	}
}
template<typename NodeData, int splitThreashold, int mergeThreadshold>
Octant Bool3D<NodeData, splitThreashold, mergeThreadshold>::getOctant(const Point3D& center, const Point3D& point)
{
	// Boolean values implicitly convert to either 1 or 0, then get bit shifted to the correct location, then merged with bitwise or.
	return (point.x() > center.x()) << 2 | (point.y() > center.y()) << 1 | (point.z() > center.z());
}