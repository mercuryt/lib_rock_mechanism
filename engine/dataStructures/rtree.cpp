#include "rtree.h"
RTree::ArrayIndex RTree::Node::offsetFor(const Index& index) const
{
	auto found = std::ranges::find(m_childIndices, index);
	assert(found != m_childIndices.end());
	return ArrayIndex::create(std::distance(m_childIndices.begin(), found));
}
void RTree::Node::updateChildIndex(const Index& oldIndex, const Index& newIndex)
{
	const auto found = std::ranges::find(m_childIndices, oldIndex);
	assert(found != m_childIndices.end());
	uint offset = std::distance(m_childIndices.begin(), found);
	m_childIndices[offset] = newIndex;
}
void RTree::Node::insertLeaf(const Cuboid& cuboid)
{
	assert(m_leafEnd != m_childBegin);
	m_cuboids.insert(m_leafEnd.get(), cuboid);
	++m_leafEnd;
}
void RTree::Node::insertBranch(const Cuboid& cuboid, const Index& index)
{
	assert(m_leafEnd != m_childBegin);
	uint toInsertAt = m_childBegin.get() - 1;
	m_childIndices[toInsertAt] = index;
	m_cuboids.insert(toInsertAt, cuboid);
	--m_childBegin;
}
void RTree::Node::eraseBranch(const ArrayIndex& offset)
{
	assert(offset < nodeSize);
	assert(offset >= m_childBegin);
	if(offset != m_childBegin)
	{
		m_cuboids.insert(offset.get(), m_cuboids[m_childBegin.get()]);
		m_childIndices[offset.get()] = m_childIndices[m_childBegin.get()];
	}
	m_cuboids.erase(m_childBegin.get());
	m_childIndices[m_childBegin.get()].clear();
	++m_childBegin;
}
void RTree::Node::eraseLeaf(const ArrayIndex& offset)
{
	assert(offset < m_leafEnd);
	ArrayIndex lastLeaf = m_leafEnd - 1;
	if(offset != lastLeaf)
	{
		m_cuboids.insert(offset.get(), m_cuboids[lastLeaf.get()]);
		m_childIndices[offset.get()] = m_childIndices[lastLeaf.get()];
	}
	m_cuboids.erase(lastLeaf.get());
	assert(m_childIndices[lastLeaf.get()].empty());
	--m_leafEnd;
}
void RTree::Node::eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
{
	ArrayIndex leafEnd = m_leafEnd;
	ArrayIndex childBegin = m_childBegin;
	const auto copyCuboids = m_cuboids;
	const auto copyChildren = m_childIndices;
	clear();
	for(ArrayIndex i = ArrayIndex::create(0); i < leafEnd; ++i)
		if(!mask[i.get()])
			insertLeaf(copyCuboids[i.get()]);
	for(ArrayIndex i = childBegin; i < nodeSize; ++i)
		if(!mask[i.get()])
			insertBranch(copyCuboids[i.get()], copyChildren[i.get()]);
}
void RTree::Node::clear()
{
	m_cuboids.clear();
	m_childIndices.fill(Index::null());
	m_leafEnd = ArrayIndex::create(0);
	m_childBegin = ArrayIndex::create(nodeSize);
}
void RTree::Node::updateLeaf(const ArrayIndex offset, const Cuboid& cuboid)
{
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
void RTree::Node::updateBranchBoundry(const ArrayIndex offset, const Cuboid& cuboid)
{
	assert(offset >= m_childBegin);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
std::string RTree::Node::toString()
{
	std::string output = "parent: " + m_parent.toString() + "; ";
	if(m_leafEnd != 0)
	{
		output += "leaves: {";
		for(ArrayIndex i = ArrayIndex::create(0); i < m_leafEnd; ++i)
			output += "(" + m_cuboids[i.get()].toString() + "), ";
		output += "}; ";
	}
	if(m_childBegin != nodeSize)
	{
		output += "children: {";
		for(ArrayIndex i = m_childBegin; i < nodeSize; ++i)
			output += "(" + m_cuboids[i.get()].toString() + ":" + m_childIndices[i.get()].toString() + "), ";
		output += "}; ";
	}
	return output;
}
std::tuple<Cuboid, RTree::ArrayIndex, RTree::ArrayIndex> RTree::findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids)
{
	// May be negitive because resulting cuboids may intersect.
	int resultEmptySpace = INT32_MAX;
	std::tuple<Cuboid, ArrayIndex, ArrayIndex> output;
	const auto endInner = cuboids.size();
	// Skip the final iteration of the outer loop, it would be redundant.
	const auto endOuter = endInner - 1;
	for(ArrayIndex firstResultIndex = ArrayIndex::create(0); firstResultIndex < endOuter; ++firstResultIndex)
	{
		for(ArrayIndex secondResultIndex = firstResultIndex + 1; secondResultIndex < endInner; ++secondResultIndex)
		{
			const Cuboid& firstCuboid = cuboids[firstResultIndex.get()];
			const Cuboid& secondCuboid = cuboids[secondResultIndex.get()];
			Cuboid sum = firstCuboid;
			sum.maybeExpand(secondCuboid);
			int emptySpace = sum.size() - (firstCuboid.size() + secondCuboid.size());
			if(resultEmptySpace > emptySpace)
			{
				resultEmptySpace = emptySpace;
				output = std::make_tuple(sum, firstResultIndex, secondResultIndex);
			}
		}
	}
	return output;
}
SmallSet<Cuboid> RTree::gatherLeavesRecursive(const Index& parent)
{
	SmallSet<Cuboid> output;
	SmallSet<Index> openList;
	openList.insert(parent);
	while(!openList.empty())
	{
		Node& node = m_nodes[openList.back()];
		openList.popBack();
		const auto endLeaf = node.getLeafCount();
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildern = node.getChildIndices();
		for(int i = 0; i != endLeaf; ++i)
			output.insert(nodeCuboids[i]);
		for(ArrayIndex i = node.offsetOfFirstChild(); i != nodeSize; ++i)
			openList.insert(nodeChildern[i.get()]);
	}
	return output;
}
void RTree::destroyWithChildren(const Index& index)
{
	SmallSet<Index> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		Index index = openList.back();
		m_emptySlots.insert(index);
		Node node = m_nodes[index];
		openList.popBack();
		for(auto i = node.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const auto& childIndex = node.getChildIndices()[i.get()];
			if(childIndex.exists())
				openList.insert(childIndex);
		}
	}
}
void RTree::tryToMergeLeaves(Node& parent)
{
	// Iterate through leaves
	ArrayIndex offset = ArrayIndex::create(0);
	const auto& parentChildren = parent.getChildIndices();
	while(offset != parent.getLeafCount())
	{
		const auto& parentCuboids = parent.getCuboids();
		const auto& mergeMask = parentCuboids.indicesOfMergeableCuboids(parentCuboids[offset.get()]);
		if(!mergeMask.any())
		{
			++offset;
			continue;
		}
		bool found = false;
		const auto& leafCount = parent.getLeafCount();
		for(ArrayIndex i = ArrayIndex::create(0); i < leafCount; ++i)
			if(mergeMask[i.get()] && parentChildren[i.get()].empty())
			{
				// Found a leaf to merge with.
				auto merged = parentCuboids[i.get()];
				merged.maybeExpand(parentCuboids[offset.get()]);
				parent.updateLeaf(i, merged);
				parent.eraseLeaf(offset);
				// Eraseing offset invalidates mergeMask. Break for now and return with next outer loop iteration.
				found = true;
				break;
			}
		if(found)
			// When a merge is made start over at offset 0.
			offset = ArrayIndex::create(0);
		else
			// If no merge is made increment the offset
			++offset;
	}
}
void RTree::clearAllContained(Node& parent, const Cuboid& cuboid)
{
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentChildIndices = parent.getChildIndices();
	const auto containsMask = parentCuboids.indicesOfContainedCuboids(cuboid);
	if(!containsMask.any())
		return;
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
		destroyWithChildren(parentChildIndices[i.get()]);
	parent.eraseByMask(containsMask);
}
void RTree::addToNodeRecursive(const Index& index, const Cuboid& cuboid)
{
	m_toComb.maybeInsert(index);
	Node& parent = m_nodes[index];
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentChildIndices = parent.getChildIndices();
	clearAllContained(parent, cuboid);
	if(parent.unusedCapacity() == 0)
	{
		// Node is full, find two cuboids to merge.
		CuboidArray<nodeSize + 1> extended;
		for(ArrayIndex i = ArrayIndex::create(0); i < nodeSize; ++i)
			extended.insert(i.get(), parentCuboids[i.get()]);
		extended.insert(nodeSize, cuboid);
		auto [mergedCuboid, firstArrayIndex, secondArrayIndex] = findPairWithLeastNewVolumeWhenExtended(extended);
		assert(firstArrayIndex >= 0);
		assert(firstArrayIndex < nodeSize);
		assert(firstArrayIndex < parent.getLeafCount() || firstArrayIndex >= parent.offsetOfFirstChild());
		assert(secondArrayIndex >= 1);
		assert(secondArrayIndex <= nodeSize);
		assert(secondArrayIndex < parent.getLeafCount() || secondArrayIndex >= parent.offsetOfFirstChild());
		const auto& leafCount = parent.getLeafCount();
		if(firstArrayIndex < leafCount)
		{
			if(secondArrayIndex == nodeSize || secondArrayIndex < leafCount)
			{
				// Both first and second cuboids are leaves.
				if(mergedCuboid.size() == extended[firstArrayIndex.get()].size() + extended[secondArrayIndex.get()].size())
				{
					// Can merge into a single leaf.
					parent.updateLeaf(firstArrayIndex, mergedCuboid);
					if(secondArrayIndex != nodeSize)
						// The newly inserted cuboid was not merged. Store it in now avalible second offset, overwriting the leaf which was merged.
						parent.updateLeaf(secondArrayIndex, cuboid);
				}
				else
				{
					// create a new node to hold first and second. Invalidates parent.
					m_nodes.add();
					Node& parent2 = m_nodes[index];
					Node& newNode = m_nodes.back();
					newNode.setParent(index);
					const Index newIndex = Index::create(m_nodes.size() - 1);
					newNode.insertLeaf(extended[firstArrayIndex.get()]);
					newNode.insertLeaf(extended[secondArrayIndex.get()]);
					parent2.eraseLeaf(firstArrayIndex);
					parent2.eraseLeaf(secondArrayIndex);
					parent2.insertBranch(mergedCuboid, newIndex);
					if(secondArrayIndex != nodeSize)
						// The newly inserted cuboid was not merged into newNode. Store it in parent.
						parent2.insertLeaf(cuboid);

				}
			}
			else
			{
				// Merge leaf at first offset location with branch at second.
				addToNodeRecursive(parentChildIndices[secondArrayIndex.get()], extended[firstArrayIndex.get()]);
				// Update the boundry for the branch.
				parent.updateBranchBoundry(secondArrayIndex, mergedCuboid);
				// Store the new cuboid in the now avalible first offset, overwriting the leaf which was merged.
				parent.updateLeaf(firstArrayIndex, cuboid);
			}
		}
		else
		{
			if(secondArrayIndex == nodeSize || secondArrayIndex < leafCount)
			{
				// Merge leaf at second offset with branch at first.
				addToNodeRecursive(parentChildIndices[firstArrayIndex.get()], extended[secondArrayIndex.get()]);
				// Update the boundry for the branch.
				parent.updateBranchBoundry(firstArrayIndex, mergedCuboid);
				if(secondArrayIndex != nodeSize)
					// The newly inserted cuboid was not merged. Store it in now avalible second offset, overwriting the leaf which was merged.
					parent.updateLeaf(secondArrayIndex, cuboid);
			}
			else
			{
				// Both first and second offsets are branches, merge second into first.
				const Index firstIndex = parentChildIndices[firstArrayIndex.get()];
				const Index secondIndex = parentChildIndices[secondArrayIndex.get()];
				merge(firstIndex, secondIndex);
				// Update the boundry for the first branch.
				parent.updateBranchBoundry(firstArrayIndex, mergedCuboid);
				// Erase the second branch and insert the leaf.
				parent.eraseBranch(secondArrayIndex);
				parent.insertLeaf(cuboid);
			}
		}
	}
	else
		parent.insertLeaf(cuboid);
}
void RTree::removeFromNode(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList)
{
	Node& parent = m_nodes[index];
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentChildIndices = parent.getChildIndices();
	auto interceptMask = parentCuboids.indicesOfIntersectingCuboids(cuboid);
	// Fragment all intercepted leaves.
	SmallSet<Cuboid> fragments;
	const auto& leafEnd = parent.getLeafCount();
	for(ArrayIndex i = ArrayIndex::create(0); i < leafEnd; ++i)
		if(interceptMask[i.get()])
			fragments.insertAll(parentCuboids[i.get()].getChildrenWhenSplitByCuboid(cuboid));
	// Copy intercept mask to create leaf intercept mask.
	auto leafInterceptMask = interceptMask;
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
		leafInterceptMask[i.get()] = 0;
	// erase all intercepted leaves.
	parent.eraseByMask(leafInterceptMask);
	// insert fragments.
	const auto end = fragments.end();
	for(auto i = fragments.begin(); i < end; ++i)
		addToNodeRecursive(index, *i);
	// Erasing and adding leaves has not invalidated intercept mask for branches.
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
	{
		if(interceptMask[i.get()])
		{
			const Index& childIndex = parentChildIndices[i.get()];
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
	m_toComb.maybeInsert(index);
}
void RTree::merge(const Index& destination, const Index& source)
{
	// TODO: maybe leave source nodes intact instead of striping out leaves?
	const auto leaves = gatherLeavesRecursive(source);
	for(const Cuboid& leaf : leaves)
		addToNodeRecursive(destination, leaf);
}
void RTree::comb()
{
	// Attempt to merge leaves and child nodes.
	// Repeat untill there are no more mergers found, then repeat with parent if it has space to merge child.
	while(!m_toComb.empty())
	{
		auto copy = std::move(m_toComb);
		m_toComb.clear();
		for(const Index& index : copy)
		{
			Node& node = m_nodes[index];
			tryToMergeLeaves(node);
			// Try to merge child branches into this branch.
			bool merged = false;
			// plus one because we are removing one as well as adding.
			uint capacity = node.unusedCapacity() + 1;
			const auto& nodeChildIndices = node.getChildIndices();
			for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			{
				// Take copy rather then reference because the original may be erased.
				const Index childIndex = nodeChildIndices[i.get()];
				Node& childNode = m_nodes[childIndex];
				uint childSize = childNode.getChildCount() + childNode.getLeafCount();
				if(childSize <= capacity)
				{
					// Child can fit into parent, copy the elements and discard.
					capacity -= childSize;
					m_emptySlots.insert(childIndex);
					node.eraseBranch(i);
					merge(index, childIndex);
					merged = true;
					// Erasing the branch invalidates i.
					// Since merged is set to true this node will be reinserted into toComb, to find any further mergers.
					break;
				}
			}
			if(merged)
				// If any node merger is found then go for another round of mergers.
				m_toComb.maybeInsert(index);
			else if(index != 0)
			{
				// If no node merger happens check if the parent can merge with this node.
				// If so add it to m_toComb to be checked next iteration.
				const Index& parentIndex = node.getParent();
				// Add one to capacity because merging will remove this branch.
				if((m_nodes[parentIndex].unusedCapacity() + 1) >= node.getChildCount() + node.getLeafCount())
					m_toComb.maybeInsert(parentIndex);
			}
		}
	}
}
void RTree::defragment()
{
	// Copy nodes from then end of m_nodes over empty slots.
	// Update parent and child indices.
	assert(m_toComb.empty());
	while(!m_emptySlots.empty())
	{
		const Index last = Index::create(m_nodes.size() - 1);
		auto found = m_emptySlots.find(last);
		if(found != m_emptySlots.end())
		{
			// Last node in m_nodes is in m_emptySlots.
			m_nodes.popBack();
			m_emptySlots.erase(found);
			continue;
		}
		const Index parentIndex = m_nodes[last].getParent();
		// Copy over the last slot in m_emptySlots.
		const Index empty = m_emptySlots.back();
		m_emptySlots.popBack();
		const auto& newNode = m_nodes[empty] = m_nodes[last];
		// Discard now copied from node.
		m_nodes.popBack();
		if(parentIndex.exists())
			// Update stored index in parent
			m_nodes[parentIndex].updateChildIndex(last, empty);

		const auto& newNodeChildren = newNode.getChildIndices();
		for(auto i = newNode.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const Index& childIndex = newNodeChildren[i.get()];
			if(childIndex.exists())
			{
				assert(m_nodes[childIndex].getParent() == last);
				m_nodes[childIndex].setParent(empty);
			}
		}
	}
}
void RTree::sort()
{
	assert(m_emptySlots.empty());
	assert(m_toComb.empty());
	StrongVector<Index, Index> indices;
	StrongVector<Node, Index> sortedNodes;
	indices.resize(m_nodes.size());
	sortedNodes.resize(m_nodes.size());
	std::iota(indices.begin(), indices.end(), Index::create(0));
	// Don't sort the first element, it is always root.
	std::ranges::sort(indices.begin() + 1, indices.end(), std::less{}, [&](const Index& index) { return m_nodes[index].sortOrder(); });
	sortedNodes[Index::create(0)] = m_nodes.front();
	const auto end = m_nodes.size();
	// Copy nodes into new order.
	for(Index oldIndex = Index::create(1); oldIndex < end; ++oldIndex)
	{
		const Index& newIndex = indices[oldIndex];
		assert(newIndex != 0);
		sortedNodes[newIndex] = m_nodes[oldIndex];
	}
	// Update copied nodes' parents and children.
	for(Index oldIndex = Index::create(1); oldIndex < end; ++oldIndex)
	{
		const Index& newIndex = indices[oldIndex];
		if(newIndex == oldIndex)
			continue;
		Node& newNode = sortedNodes[newIndex];
		sortedNodes[newNode.getParent()].updateChildIndex(oldIndex, newIndex);
		const auto& newNodeChildren = newNode.getChildIndices();
		for(ArrayIndex i = newNode.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const Index& childIndex = newNodeChildren[i.get()];
			assert(sortedNodes[childIndex].getParent() == oldIndex);
			sortedNodes[childIndex].setParent(newIndex);
		}
	}
	m_nodes = std::move(sortedNodes);
}
void RTree::maybeInsert(const Cuboid& cuboid)
{
	constexpr Index zeroIndex = Index::create(0);
	addToNodeRecursive(zeroIndex, cuboid);
}
void RTree::maybeRemove(const Cuboid& cuboid)
{
	// Erase all contained branches and leaves.
	constexpr Index rootIndex = Index::create(0);
	clearAllContained(m_nodes[rootIndex], cuboid);
	SmallSet<Index> openList;
	SmallSet<Index> toUpdateBoundryMaybe;
	openList.insert(rootIndex);
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		removeFromNode(index, cuboid, openList);
		if(index != 0)
			toUpdateBoundryMaybe.maybeInsert(index);
	}
	for(const Index& index : toUpdateBoundryMaybe)
	{
		Node& node = m_nodes[index];
		Node& parent = m_nodes[node.getParent()];
		const ArrayIndex offset = parent.offsetFor(index);
		parent.updateBranchBoundry(offset, node.getCuboids().boundry());
	}
}
void RTree::prepare()
{
	bool toSort = false;
	if(!m_toComb.empty())
	{
		toSort = true;
		comb();
	}
	if(!m_emptySlots.empty())
	{
		toSort = true;
		defragment();
	}
	if(toSort)
		sort();
}
bool RTree::queryPoint(const Point3D& point) const
{
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildren = node.getChildIndices();
		const auto interceptMask = nodeCuboids.indicesOfIntersectingCuboids(point);
		const auto leafCount = node.getLeafCount();
		if(interceptMask.segment(0, leafCount).any())
			return true;
		for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			if(interceptMask[i.get()])
				// Cuboid intercepts existing branch, add it to open list for a future iteration.
				openList.insert(nodeChildren[i.get()]);
	}
	return false;
}
bool RTree::queryCuboid(const Cuboid& cuboid) const
{
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildren = node.getChildIndices();
		const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(cuboid);
		const auto leafCount = node.getLeafCount();
		if(interceptMask.segment(0, leafCount).any())
			return true;
		for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			if(interceptMask[i.get()])
				// Cuboid intercepts existing branch, add it to open list for a future iteration.
				openList.insert(nodeChildren[i.get()]);
	}
	return false;
}
bool RTree::querySphere(const Sphere& sphere) const
{
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildren = node.getChildIndices();
		const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(sphere);
		const auto leafCount = node.getLeafCount();
		if(interceptMask.segment(0, leafCount).any())
			return true;
		for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			if(interceptMask[i.get()])
				// Cuboid intercepts existing branch, add it to open list for a future iteration.
				openList.insert(nodeChildren[i.get()]);
	}
	return false;
}
bool RTree::queryLine(const Point3D& point1, const Point3D& point2) const
{
	if(point1 < point2)
		return queryLine(point2, point1);
	Point3D difference = point1 - point2;
	DistanceInBlocksFractional distance = point1.distanceToFractional(point2);
	Eigen::Array<float, 1, 3> sloap = difference.data.cast<float>();
	sloap /= distance.get();
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		Index index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildren = node.getChildIndices();
		const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboidsForLine(point1, point2, sloap);
		const auto leafCount = node.getLeafCount();
		if(interceptMask.segment(0, leafCount).any())
			return true;
		for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			if(interceptMask[i.get()])
				// Cuboid intercepts existing branch, add it to open list for a future iteration.
				openList.insert(nodeChildren[i.get()]);
	}
	return false;
}
uint RTree::leafCount() const
{
	uint output = 0;
	for(Index i = Index::create(0); i < m_nodes.size(); ++i)
	{
		if(m_emptySlots.contains(i))
			continue;
		const Node& node = m_nodes[i];
		output += node.getLeafCount();
	}
	return output;
}
const RTree::Node& RTree::getNode(uint i) const { return m_nodes[Index::create(i)]; }
const Cuboid RTree::getNodeCuboid(uint i, uint o) const { return m_nodes[Index::create(i)].getCuboids()[o]; }
const RTree::Index& RTree::getNodeChild(uint i, uint o) const { return m_nodes[Index::create(i)].getChildIndices()[o]; }
bool RTree::queryPoint(uint x, uint y, uint z) const { return queryPoint(Point3D::create(x,y,z)); }
uint RTree::getNodeSize() { return nodeSize; }
