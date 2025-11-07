#include "rtreeBoolean.h"
#include "strongVector.hpp"
#include "../geometry/paramaterizedLine.h"
RTreeBoolean::ArrayIndex RTreeBoolean::Node::offsetFor(const Index& index) const
{
	auto found = std::ranges::find(m_childIndices.begin() + m_childBegin.get(), m_childIndices.end(), index);
	assert(found != m_childIndices.end());
	return ArrayIndex::create(std::distance(m_childIndices.begin(), found));
}
uint RTreeBoolean::Node::getLeafVolume() const
{
	uint output = 0;
	for(auto i = ArrayIndex::create(0); i < m_leafEnd; ++i)
		output += m_cuboids[i.get()].volume();
	return output;
}
uint RTreeBoolean::Node::getNodeVolume() const
{
	uint output = 0;
	for(auto i = m_childBegin; i < nodeSize; ++i)
		output += m_cuboids[i.get()].volume();
	return output;
}
CuboidSet RTreeBoolean::Node::getLeafCuboids() const
{
	CuboidSet output;
	for(auto i = ArrayIndex::create(0); i < m_leafEnd; ++i)
		output.add(m_cuboids[i.get()]);
	return output;
}
void RTreeBoolean::Node::updateChildIndex(const Index& oldIndex, const Index& newIndex)
{
	ArrayIndex offset = offsetFor(oldIndex);
	m_childIndices[offset.get()] = newIndex;
}
void RTreeBoolean::Node::insertLeaf(const Cuboid& cuboid)
{
	assert(m_leafEnd != m_childBegin);
	m_cuboids.insert(m_leafEnd.get(), cuboid);
	++m_leafEnd;
}
void RTreeBoolean::Node::insertBranch(const Cuboid& cuboid, const Index& index)
{
	assert(m_leafEnd != m_childBegin);
	uint toInsertAt = m_childBegin.get() - 1;
	m_childIndices[toInsertAt] = index;
	m_cuboids.insert(toInsertAt, cuboid);
	--m_childBegin;
}
void RTreeBoolean::Node::eraseBranch(const ArrayIndex& offset)
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
void RTreeBoolean::Node::eraseLeaf(const ArrayIndex& offset)
{
	assert(offset < m_leafEnd);
	ArrayIndex lastLeaf = m_leafEnd - 1;
	if(offset != lastLeaf)
		m_cuboids.insert(offset.get(), m_cuboids[lastLeaf.get()]);
	m_cuboids.erase(lastLeaf.get());
	assert(m_childIndices[lastLeaf.get()].empty());
	assert(m_childIndices[offset.get()].empty());
	--m_leafEnd;
}
void RTreeBoolean::Node::eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
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
void RTreeBoolean::Node::eraseLeavesByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
{
	ArrayIndex leafEnd = m_leafEnd;
	const auto copyCuboids = m_cuboids;
	// Overwrite from copy.
	m_leafEnd = ArrayIndex::create(0);
	for(ArrayIndex i = ArrayIndex::create(0); i < leafEnd; ++i)
		if(!mask[i.get()])
			insertLeaf(copyCuboids[i.get()]);
	// Clear any remaning leaves.
	for(auto i = m_leafEnd; i < leafEnd; ++i)
		m_cuboids.erase(i.get());
}
void RTreeBoolean::Node::clear()
{
	m_cuboids.clear();
	m_childIndices.fill(Index::null());
	m_leafEnd = ArrayIndex::create(0);
	m_childBegin = ArrayIndex::create(nodeSize);
}
void RTreeBoolean::Node::updateLeaf(const ArrayIndex& offset, const Cuboid& cuboid)
{
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
void RTreeBoolean::Node::updateBranchBoundry(const ArrayIndex& offset, const Cuboid& cuboid)
{
	assert(offset >= m_childBegin);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
std::string RTreeBoolean::Node::toString()
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
void RTreeBoolean::beforeJsonLoad() { m_nodes.clear(); }
std::tuple<Cuboid, RTreeBoolean::ArrayIndex, RTreeBoolean::ArrayIndex> RTreeBoolean::findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const
{
	// May be negitive because resulting cuboids may intersect.
	int resultEmptySpace = INT32_MAX;
	uint resultSize = UINT32_MAX;
	std::tuple<Cuboid, ArrayIndex, ArrayIndex> output;
	const auto endInner = nodeSize + 1;
	// Skip the final iteration of the outer loop, it would be redundant.
	const auto endOuter = nodeSize;
	for(ArrayIndex firstResultIndex = ArrayIndex::create(0); firstResultIndex < endOuter; ++firstResultIndex)
	{
		for(ArrayIndex secondResultIndex = firstResultIndex + 1; secondResultIndex < endInner; ++secondResultIndex)
		{
			const Cuboid& firstCuboid = cuboids[firstResultIndex.get()];
			const Cuboid& secondCuboid = cuboids[secondResultIndex.get()];
			Cuboid sum = firstCuboid;
			sum.maybeExpand(secondCuboid);
			const uint size = sum.volume();
			int emptySpace = size - (firstCuboid.volume() + secondCuboid.volume());
			// If empty space is equal hen prefer the smaller result.
			// TODO: This may be counter productive.
			if(resultEmptySpace > emptySpace || (resultEmptySpace == emptySpace && resultSize > size))
			{
				resultEmptySpace = emptySpace;
				resultSize = size;
				output = std::make_tuple(sum, firstResultIndex, secondResultIndex);
			}
		}
	}
	return output;
}
SmallSet<Cuboid> RTreeBoolean::gatherLeavesRecursive(const Index& parent) const
{
	SmallSet<Cuboid> output;
	SmallSet<Index> openList;
	openList.insert(parent);
	while(!openList.empty())
	{
		const Node& node = m_nodes[openList.back()];
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
void RTreeBoolean::destroyWithChildren(const Index& index)
{
	SmallSet<Index> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		Index current = openList.back();
		m_emptySlots.insert(current);
		Node& node = m_nodes[current];
		const auto& children = node.getChildIndices();
		openList.popBack();
		for(auto i = node.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const auto& childIndex = children[i.get()];
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
}
void RTreeBoolean::tryToMergeLeaves(Node& parent)
{
	// Iterate through leaves
	ArrayIndex offset = ArrayIndex::create(0);
	const auto& parentCuboids = parent.getCuboids();
	while(offset != parent.getLeafCount())
	{
		const auto& mergeMask = parentCuboids.indicesOfMergeableCuboids(parentCuboids[offset.get()]);
		if(!mergeMask.any())
		{
			++offset;
			continue;
		}
		bool found = false;
		const auto& leafCount = parent.getLeafCount();
		for(ArrayIndex i = ArrayIndex::create(0); i < leafCount; ++i)
			if(mergeMask[i.get()])
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
void RTreeBoolean::clearAllContained(const Index& index, const Cuboid& cuboid)
{
	SmallSet<Index> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		Node& node = m_nodes[openList.back()];
		openList.popBack();
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildIndices = node.getChildIndices();
		// Destroy contained branches.
		const auto containsMask = nodeCuboids.indicesOfContainedCuboids(cuboid);
		for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			if(containsMask[i.get()])
				destroyWithChildren(nodeChildIndices[i.get()]);
		// Erase contained cuboids, both leaf and branch.
		node.eraseByMask(containsMask);
		// Gather branches intercted with to add to openList.
		const auto interceptMask = nodeCuboids.indicesOfIntersectingCuboids(cuboid);
		if(interceptMask.any())
		{
			const auto begin = interceptMask.begin();
			const auto end = interceptMask.end();
			for(auto iter = begin + node.offsetOfFirstChild().get(); iter != end; ++iter)
				if(*iter)
				{
					ArrayIndex iterIndex = ArrayIndex::create(iter - begin);
					openList.insert(nodeChildIndices[iterIndex.get()]);
				}
		}
	}
}
void RTreeBoolean::addToNodeRecursive(const Index& index, const Cuboid& cuboid)
{
	m_toComb.maybeInsert(index);
	Node& parent = m_nodes[index];
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentChildIndices = parent.getChildIndices();
	clearAllContained(index, cuboid);
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
		const bool newCuboidIsSecondIndex = secondArrayIndex == nodeSize;
		if(firstArrayIndex < leafCount)
		{
			if(newCuboidIsSecondIndex || secondArrayIndex < leafCount)
			{
				// Both first and second cuboids are leaves.
				if(mergedCuboid.volume() == extended[firstArrayIndex.get()].volume() + extended[secondArrayIndex.get()].volume())
				{
					// Can merge into a single leaf.
					parent.updateLeaf(firstArrayIndex, mergedCuboid);
					if(!newCuboidIsSecondIndex)
						// The newly inserted cuboid was not merged. Store it in now avalible second index, overwriting the leaf which was merged.
						parent.updateLeaf(secondArrayIndex, cuboid);
				}
				else
				{
					const Index indexCopy = index;
					// create a new node to hold first and second. Invalidates parent and index.
					m_nodes.add();
					Node& newNode = m_nodes.back();
					newNode.setParent(indexCopy);
					const Index newIndex = Index::create(m_nodes.size() - 1);
					newNode.insertLeaf(extended[firstArrayIndex.get()]);
					newNode.insertLeaf(extended[secondArrayIndex.get()]);
					Node& parent2 = m_nodes[indexCopy];
					if(!newCuboidIsSecondIndex)
						// The new branch does not contain cuboid, overwrite the leaf at the second index with it.
						parent2.updateLeaf(secondArrayIndex, cuboid);
					// Invalidates array indices.
					parent2.eraseLeaf(firstArrayIndex);
					parent2.insertBranch(mergedCuboid, newIndex);

				}
			}
			else
			{
				// Merge leaf at first index with branch at second.
				// Update the boundry for the branch.
				parent.updateBranchBoundry(secondArrayIndex, mergedCuboid);
				// Store the new cuboid in the now avalible first offset, overwriting the leaf which was merged.
				parent.updateLeaf(firstArrayIndex, cuboid);
				// Invalidates array indices.
				addToNodeRecursive(parentChildIndices[secondArrayIndex.get()], extended[firstArrayIndex.get()]);
			}
		}
		else
		{
			if(newCuboidIsSecondIndex || secondArrayIndex < leafCount)
			{
				// Merge leaf from second index with branch at first.
				// Update the boundry for the branch.
				parent.updateBranchBoundry(firstArrayIndex, mergedCuboid);
				if(!newCuboidIsSecondIndex)
					// The newly inserted cuboid was not merged. Store it in now avalible second offset, overwriting the leaf which will be merged.
					parent.updateLeaf(secondArrayIndex, cuboid);
				// Invalidates array indices.
				addToNodeRecursive(parentChildIndices[firstArrayIndex.get()], extended[secondArrayIndex.get()]);
			}
			else
			{
				// Both first and second offsets are branches, merge second into first.
				const Index firstIndex = parentChildIndices[firstArrayIndex.get()];
				const Index secondIndex = parentChildIndices[secondArrayIndex.get()];
				const Index indexCopy = index;
				// Invalidates parent and index.
				merge(firstIndex, secondIndex);
				Node& parent2 = m_nodes[indexCopy];
				// Update the boundry for the first branch.
				parent2.updateBranchBoundry(firstArrayIndex, mergedCuboid);
				// Erase the second branch. Invalidates array indices.
				parent2.eraseBranch(secondArrayIndex);
				// Mark second branch node and all children for removal.
				destroyWithChildren(secondIndex);
				// Use now open slot to store new leaf.
				parent2.insertLeaf(cuboid);
			}
		}
	}
	else
		parent.insertLeaf(cuboid);
}
void RTreeBoolean::removeFromNode(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList)
{
	Node& parent = m_nodes[index];
	const auto& parentCuboids = parent.getCuboids();
	auto interceptMask = parentCuboids.indicesOfIntersectingCuboids(cuboid);
	// Fragment all intercepted leaves.
	SmallSet<Cuboid> fragments;
	const auto& leafEnd = parent.getLeafCount();
	for(ArrayIndex i = ArrayIndex::create(0); i < leafEnd; ++i)
		if(interceptMask[i.get()])
			fragments.insertAll(parentCuboids[i.get()].getChildrenWhenSplitByCuboid(cuboid));
	// Copy intercept mask to create leaf intercept mask.
	auto leafInterceptMask = interceptMask;
	leafInterceptMask.tail(parent.getChildCount()) = 0;
	// erase all intercepted leaves.
	parent.eraseLeavesByMask(leafInterceptMask);
	const Index index2 = index;
	// insert fragments. Invalidates parent and cuboids, as well as index.
	const auto end = fragments.end();
	for(auto i = fragments.begin(); i < end; ++i)
		addToNodeRecursive(index2, *i);
	Node& parent2 = m_nodes[index2];
	const auto& parentChildIndices2 = parent2.getChildIndices();
	// Erasing and adding leaves has not invalidated intercept mask for branches.
	for(ArrayIndex i = parent2.offsetOfFirstChild(); i < nodeSize; ++i)
	{
		if(interceptMask[i.get()])
		{
			const Index& childIndex = parentChildIndices2[i.get()];
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
	m_toComb.maybeInsert(index2);
}
void RTreeBoolean::merge(const Index& destination, const Index& source)
{
	// TODO: maybe leave source nodes intact instead of striping out leaves?
	const auto leaves = gatherLeavesRecursive(source);
	for(const Cuboid& leaf : leaves)
		addToNodeRecursive(destination, leaf);
}
void RTreeBoolean::comb()
{
	// Attempt to merge leaves and child nodes.
	// Don't comb empty slots.
	m_toComb.sort();
	m_emptySlots.sort();
	m_toComb.maybeEraseAllWhereBothSetsAreSorted(m_emptySlots);
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
void RTreeBoolean::defragment()
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
void RTreeBoolean::sort()
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
void RTreeBoolean::maybeInsert(const Cuboid& cuboid)
{
	constexpr Index zeroIndex = Index::create(0);
	addToNodeRecursive(zeroIndex, cuboid);
}
void RTreeBoolean::maybeRemove(const Cuboid& cuboid)
{
	// Erase all contained branches and leaves.
	constexpr Index rootIndex = Index::create(0);
	clearAllContained(rootIndex, cuboid);
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
		const Index& parentIndex = node.getParent();
		Node& parent = m_nodes[parentIndex];
		const ArrayIndex offset = parent.offsetFor(index);
		if(node.empty())
		{
			parent.eraseBranch(offset);
			m_toComb.maybeInsert(parentIndex);
			m_emptySlots.insert(index);
		}
		else
			parent.updateBranchBoundry(offset, node.getCuboids().boundry());
	}
}
void RTreeBoolean::clear()
{
	m_nodes.resize(1);
	m_emptySlots.clear();
	m_toComb.clear();
	m_nodes.front().clear();
}
void RTreeBoolean::prepare()
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
bool RTreeBoolean::query(const Point3D& begin, const Point3D& end) const { return query(ParamaterizedLine(begin, end)); }
CuboidSet RTreeBoolean::toCuboidSet() const
{
	CuboidSet output;
	for(Index i = Index::create(0); i < m_nodes.size(); ++i)
	{
		if(m_emptySlots.contains(i))
			continue;
		const Node& node = m_nodes[i];
		output.addSet(node.getLeafCuboids());
	}
	return output;
}
uint RTreeBoolean::leafCount() const
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
const RTreeBoolean::Node& RTreeBoolean::getNode(uint i) const { return m_nodes[Index::create(i)]; }
const Cuboid RTreeBoolean::getNodeCuboid(uint i, uint o) const { return m_nodes[Index::create(i)].getCuboids()[o]; }
const RTreeBoolean::Index& RTreeBoolean::getNodeChild(uint i, uint o) const { return m_nodes[Index::create(i)].getChildIndices()[o]; }
bool RTreeBoolean::queryPoint(uint x, uint y, uint z) const { return query(Point3D::create(x,y,z)); }
uint RTreeBoolean::totalNodeVolume() const
{
	uint output = 0;
	const auto end = m_nodes.size();
	for(Index index = Index::create(0); index < end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getNodeVolume();
	return output;
}
uint RTreeBoolean::totalLeafVolume() const
{
	uint output = 0;
	const auto end = m_nodes.size();
	for(Index index = Index::create(0); index < end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getLeafVolume();
	return output;
}
uint RTreeBoolean::getNodeSize() { return nodeSize; }
void RTreeBoolean::assertAllLeafsAreUnique() const
{
	// Will assert fail if any leaves are not unique.
	[[maybe_unused]] const auto leaves = gatherLeavesRecursive(Index::create(0));
}
// Template methods.
template<typename ShapeT>
bool RTreeBoolean::query(const ShapeT& shape) const
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
		if(leafCount != 0 && interceptMask.head(leafCount).any())
			return true;
		// TODO: Would it be better to check the whole intercept mask and continue if all are empty before checking leafs?
		const auto childCount = node.getChildCount();
		if(childCount == 0 || !interceptMask.tail(childCount).any())
			continue;
		const auto& nodeChildren = node.getChildIndices();
		const auto offsetOfFirstChild = node.offsetOfFirstChild();
		auto begin = interceptMask.begin();
		auto end = begin + nodeSize;
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
	return false;
}
template bool RTreeBoolean::query(const Point3D& shape) const;
template bool RTreeBoolean::query(const Cuboid& shape) const;
template bool RTreeBoolean::query(const CuboidSet& shape) const;
template bool RTreeBoolean::query(const Sphere& shape) const;
template bool RTreeBoolean::query(const ParamaterizedLine& shape) const;

template<typename ShapeT>
Cuboid RTreeBoolean::queryGetLeaf(const ShapeT& shape) const
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
					return nodeCuboids[iter - begin];
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
template Cuboid RTreeBoolean::queryGetLeaf(const Point3D& shape) const;
template Cuboid RTreeBoolean::queryGetLeaf(const Cuboid& shape) const;
template Cuboid RTreeBoolean::queryGetLeaf(const CuboidSet& shape) const;
template Cuboid RTreeBoolean::queryGetLeaf(const Sphere& shape) const;
template Cuboid RTreeBoolean::queryGetLeaf(const ParamaterizedLine& shape) const;

template<typename ShapeT>
Point3D RTreeBoolean::queryGetPoint(const ShapeT& shape) const
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
					return nodeCuboids[iter - begin].intersectionPoint(shape);
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
	return Point3D::null();
}
template Point3D RTreeBoolean::queryGetPoint(const Cuboid& shape) const;
template Point3D RTreeBoolean::queryGetPoint(const CuboidSet& shape) const;
template<typename ShapeT>
std::vector<bool> RTreeBoolean::batchQuery(const ShapeT& shapes) const
{
	std::vector<bool> output;
	output.resize(shapes.size());
	SmallMap<Index, SmallSet<uint>> openlist;
	openlist.insert(Index::create(0), {});
	auto& rootnodecandidatelist = openlist.back().second;
	rootnodecandidatelist.resize(shapes.size());
	std::ranges::iota(rootnodecandidatelist.getVector(), 0);
	while(!openlist.empty())
	{
		auto& [index, candidates] = openlist.back();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildren = node.getChildIndices();
		const auto offsetOfFirstChild = node.offsetOfFirstChild();
		for(const uint& shapeIndex : candidates)
		{
			if(output[shapeIndex])
				// this shape has already intersected with a leaf, no need to check further.
				continue;
			const auto& interceptmask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
			// record a leaf intersection.
			const auto leafCount = node.getLeafCount();
			if(leafCount != 0 && interceptmask.head(leafCount).any())
			{
				output[shapeIndex] = true;
				continue;
			}
			// record all node intersections.
			const auto childcount = node.getChildCount();
			if(childcount == 0 || !interceptmask.tail(childcount).any())
				continue;
			auto begin = interceptmask.begin();
			const auto end = begin + nodeSize;
			if(offsetOfFirstChild == 0)
			{
				// unrollable version for nodes where every slot is a child.
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openlist.getOrCreate(nodeChildren[iter - begin]).insert(shapeIndex);
			}
			else
			{
				for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
					if(*iter)
						openlist.getOrCreate(nodeChildren[iter - begin]).insert(shapeIndex);
			}
		}
		// pop back at the end so we can use candidates as a reference and not have to make a move-copy.
		openlist.popBack();
	}
	return output;
}
template std::vector<bool> RTreeBoolean::batchQuery(const SmallSet<Point3D>& shapes) const;
template std::vector<bool> RTreeBoolean::batchQuery(const CuboidSet& shapes) const;
template std::vector<bool> RTreeBoolean::batchQuery(const std::vector<ParamaterizedLine>& shapes) const;
template<typename ShapeT>
CuboidSet RTreeBoolean::queryGetLeaves(const ShapeT& shape) const
{
	CuboidSet output;
	SmallSet<Index> openList;
	openList.insert(Index::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& interceptMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
		const auto begin = interceptMask.begin();
		const auto leafCount = node.getLeafCount();
		const auto leafEnd = begin + leafCount;
		if(leafCount != 0 && interceptMask.head(leafCount).any())
			for(auto iter = begin; iter != leafEnd; ++iter)
				if(*iter)
					output.add(nodeCuboids[iter - begin]);
		// TODO: Would it be better to check the whole intercept mask and continue if all are empty before checking leafs?
		const auto childCount = node.getChildCount();
		if(childCount == 0 || !interceptMask.tail(childCount).any())
			continue;
		const auto& nodeChildren = node.getChildIndices();
		const auto offsetOfFirstChild = node.offsetOfFirstChild();
		const auto end = begin + nodeSize;
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
	return output;
}
template CuboidSet RTreeBoolean::queryGetLeaves(const Cuboid& shape) const;
template CuboidSet RTreeBoolean::queryGetLeaves(const CuboidSet& shape) const;
template<typename ShapeT>
CuboidSet RTreeBoolean::queryGetIntersection(const ShapeT& shape) const
{
	CuboidSet output;
	if constexpr(std::is_same_v<ShapeT, CuboidSet>)
	{
		for(const auto& subShape : shape)
			for(const Cuboid& leaf : queryGetLeaves(subShape))
				output.add(leaf.intersection(subShape));
	}
	else
		for(const Cuboid& leaf : queryGetLeaves(shape))
			output.add(leaf.intersection(shape));
	return output;
}
template CuboidSet RTreeBoolean::queryGetIntersection(const Cuboid& shape) const;
template CuboidSet RTreeBoolean::queryGetIntersection(const CuboidSet& shape) const;