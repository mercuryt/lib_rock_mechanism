#include "rtreeBoolean.h"
#include "strongVector.hpp"
#include "../geometry/paramaterizedLine.h"
RTreeArrayIndex RTreeBoolean::Node::offsetFor(const RTreeNodeIndex& index) const
{
	return RTreeArrayIndex::create(m_childIndices.indexOf(index));
}
int RTreeBoolean::Node::getLeafVolume() const
{
	int output = 0;
	for(auto i = RTreeArrayIndex::create(0); i < m_leafEnd; ++i)
		output += m_cuboids[i.get()].volume();
	return output;
}
int RTreeBoolean::Node::getNodeVolume() const
{
	int output = 0;
	for(auto i = m_childBegin; i < nodeSize; ++i)
		output += m_cuboids[i.get()].volume();
	return output;
}
CuboidSet RTreeBoolean::Node::getLeafCuboids() const
{
	CuboidSet output;
	for(auto i = RTreeArrayIndex::create(0); i < m_leafEnd; ++i)
		output.maybeAdd(m_cuboids[i.get()]);
	return output;
}
bool RTreeBoolean::Node::hasChildren() const
{
	return m_childBegin != nodeSize;
}
void RTreeBoolean::Node::updateChildIndex(const RTreeNodeIndex& oldIndex, const RTreeNodeIndex& newIndex)
{
	RTreeArrayIndex offset = offsetFor(oldIndex);
	m_childIndices[offset] = newIndex;
}
void RTreeBoolean::Node::insertLeaf(const Cuboid& cuboid)
{
	assert(m_leafEnd != m_childBegin);
	m_cuboids.insert(m_leafEnd.get(), cuboid);
	++m_leafEnd;
}
void RTreeBoolean::Node::insertBranch(const Cuboid& cuboid, const RTreeNodeIndex& index)
{
	assert(m_leafEnd != m_childBegin);
	RTreeArrayIndex toInsertAt = m_childBegin - 1;
	m_childIndices[toInsertAt] = index;
	m_cuboids.insert(toInsertAt.get(), cuboid);
	--m_childBegin;
}
void RTreeBoolean::Node::eraseBranch(const RTreeArrayIndex& offset)
{
	assert(offset < nodeSize);
	assert(offset >= m_childBegin);
	if(offset != m_childBegin)
	{
		m_cuboids.insert(offset.get(), m_cuboids[m_childBegin.get()]);
		m_childIndices[offset] = m_childIndices[m_childBegin];
	}
	m_cuboids.erase(m_childBegin.get());
	m_childIndices[m_childBegin].clear();
	++m_childBegin;
}
void RTreeBoolean::Node::eraseLeaf(const RTreeArrayIndex& offset)
{
	assert(offset < m_leafEnd);
	RTreeArrayIndex lastLeaf = m_leafEnd - 1;
	if(offset != lastLeaf)
		m_cuboids.insert(offset.get(), m_cuboids[lastLeaf.get()]);
	m_cuboids.erase(lastLeaf.get());
	assert(m_childIndices[lastLeaf].empty());
	assert(m_childIndices[offset].empty());
	--m_leafEnd;
}
void RTreeBoolean::Node::eraseByMask(BitSet& mask)
{
	RTreeArrayIndex leafEnd = m_leafEnd;
	RTreeArrayIndex childBegin = m_childBegin;
	const auto copyCuboids = m_cuboids;
	const auto copyChildren = m_childIndices;
	// Flip list of indices to remove into list to keep.
	mask.flip();
	clear();
	if(leafEnd != 0)
	{
		BitSet leafMask = mask;
		leafMask.clearAllAfterInclusive(leafEnd.get());
		while(leafMask.any())
		{
			const RTreeArrayIndex arrayIndex{leafMask.getNextAndClear()};
			insertLeaf(copyCuboids[arrayIndex.get()]);
		}
	}
	if(childBegin != nodeSize)
	{
		mask.clearAllBefore(childBegin.get());
		while(mask.any())
		{
			const RTreeArrayIndex arrayIndex{mask.getNextAndClear()};
			insertBranch(copyCuboids[arrayIndex.get()], copyChildren[arrayIndex]);
		}
	}
}
void RTreeBoolean::Node::eraseLeavesByMask(BitSet mask)
{
	RTreeArrayIndex leafEnd = m_leafEnd;
	const auto copyCuboids = m_cuboids;
	// Overwrite from copy.
	m_leafEnd = RTreeArrayIndex::create(0);
	// Flip from a list to remove into a list to keep.
	mask.flip();
	if(leafEnd != nodeSize)
		mask.clearAllAfterInclusive(leafEnd.get());
	while(mask.any())
	{
		const RTreeArrayIndex arrayIndex{mask.getNextAndClear()};
		insertLeaf(copyCuboids[arrayIndex.get()]);
	}
	// Clear any remaning leaves.
	for(auto i = m_leafEnd; i < leafEnd; ++i)
		m_cuboids.erase(i.get());
}
void RTreeBoolean::Node::clear()
{
	m_cuboids.clear();
	m_childIndices.fill(RTreeNodeIndex::null());
	m_leafEnd = RTreeArrayIndex::create(0);
	m_childBegin = RTreeArrayIndex::create(nodeSize);
}
void RTreeBoolean::Node::updateLeaf(const RTreeArrayIndex& offset, const Cuboid& cuboid)
{
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
void RTreeBoolean::Node::updateBranchBoundry(const RTreeArrayIndex& offset, const Cuboid& cuboid)
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
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < m_leafEnd; ++i)
			output += "(" + m_cuboids[i.get()].toString() + "), ";
		output += "}; ";
	}
	if(m_childBegin != nodeSize)
	{
		output += "children: {";
		for(RTreeArrayIndex i = m_childBegin; i < nodeSize; ++i)
			output += "(" + m_cuboids[i.get()].toString() + ":" + m_childIndices[i].toString() + "), ";
		output += "}; ";
	}
	return output;
}
void RTreeBoolean::beforeJsonLoad() { m_nodes.clear(); }
std::tuple<Cuboid, RTreeArrayIndex, RTreeArrayIndex> RTreeBoolean::findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const
{
	// May be negitive because resulting cuboids may intersect.
	int resultEmptySpace = INT32_MAX;
	int resultSize = INT32_MAX;
	std::tuple<Cuboid, RTreeArrayIndex, RTreeArrayIndex> output;
	const auto endInner = nodeSize + 1;
	// Skip the final iteration of the outer loop, it would be redundant.
	const auto endOuter = nodeSize;
	for(RTreeArrayIndex firstResultIndex = RTreeArrayIndex::create(0); firstResultIndex < endOuter; ++firstResultIndex)
	{
		for(RTreeArrayIndex secondResultIndex = firstResultIndex + 1; secondResultIndex < endInner; ++secondResultIndex)
		{
			const Cuboid& firstCuboid = cuboids[firstResultIndex.get()];
			const Cuboid& secondCuboid = cuboids[secondResultIndex.get()];
			Cuboid sum = firstCuboid;
			sum.maybeExpand(secondCuboid);
			const int size = sum.volume();
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
SmallSet<Cuboid> RTreeBoolean::gatherLeavesRecursive(const RTreeNodeIndex& parent) const
{
	SmallSet<Cuboid> output;
	SmallSet<RTreeNodeIndex> openList;
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
		for(RTreeArrayIndex i = node.offsetOfFirstChild(); i != nodeSize; ++i)
			openList.insert(nodeChildern[i]);
	}
	return output;
}
void RTreeBoolean::destroyWithChildren(const RTreeNodeIndex& index)
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		RTreeNodeIndex current = openList.back();
		m_emptySlots.insert(current);
		Node& node = m_nodes[current];
		const auto& children = node.getChildIndices();
		openList.popBack();
		for(auto i = node.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const auto& childIndex = children[i];
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
}
void RTreeBoolean::tryToMergeLeaves(Node& parent)
{
	// Iterate through leaves
	RTreeArrayIndex offset = RTreeArrayIndex::create(0);
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
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafCount; ++i)
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
			offset = RTreeArrayIndex::create(0);
	else
			// If no merge is made increment the offset
			++offset;
	}
}
void RTreeBoolean::clearAllContained(const RTreeNodeIndex& index, const Cuboid& cuboid)
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		Node& node = m_nodes[openList.back()];
		openList.popBack();
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildIndices = node.getChildIndices();
		const auto containsMask = nodeCuboids.indicesOfContainedCuboids(cuboid);
		if(containsMask.any())
		{
			// Destroy contained branches.
			BitSet containedBitMask = BitSet::create(containsMask);
			// Copy bitset before destructive iteration to pass onto node.eraseByMask.
			// TODO: combine these two interations.
			const auto& offsetOfFirstChild = node.offsetOfFirstChild();
			if(offsetOfFirstChild != nodeSize)
			{
				BitSet childBitMask = containedBitMask;
				childBitMask.clearAllBefore(offsetOfFirstChild.get());
				while(childBitMask.any())
				{
					const RTreeArrayIndex containedChildIndex{childBitMask.getNextAndClear()};
					destroyWithChildren(nodeChildIndices[containedChildIndex]);
				}
			}
			// Erase contained cuboids, both leaf and branch.
			node.eraseByMask(containedBitMask);
		}
		// Gather branches intersected to add to openList.
		// Leaves which are intersected but not contained are ignored.
		auto intersectMask = nodeCuboids.indicesOfIntersectingCuboids(cuboid);
		intersectMask.head(node.getLeafCount()) = false;
		if(intersectMask.any())
		{
			auto bitset = BitSet::create(intersectMask);
			addIntersectedChildrenToOpenList(node, bitset, openList);
		}
	}
}
void RTreeBoolean::addToNodeRecursive(const RTreeNodeIndex& index, const Cuboid& cuboid)
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
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < nodeSize; ++i)
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
					const RTreeNodeIndex indexCopy = index;
					// create a new node to hold first and second. Invalidates parent and index.
					m_nodes.add();
					Node& newNode = m_nodes.back();
					newNode.setParent(indexCopy);
					const RTreeNodeIndex newIndex = RTreeNodeIndex::create(m_nodes.size() - 1);
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
				addToNodeRecursive(parentChildIndices[secondArrayIndex], extended[firstArrayIndex.get()]);
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
				addToNodeRecursive(parentChildIndices[firstArrayIndex], extended[secondArrayIndex.get()]);
			}
			else
			{
				// Both first and second offsets are branches, merge second into first.
				const RTreeNodeIndex firstIndex = parentChildIndices[firstArrayIndex];
				const RTreeNodeIndex secondIndex = parentChildIndices[secondArrayIndex];
				const RTreeNodeIndex indexCopy = index;
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
void RTreeBoolean::removeFromNode(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList)
{
	const RTreeNodeIndex index2 = index;
	{
		Node& parent = m_nodes[index];
		const auto& parentCuboids = parent.getCuboids();
		auto intersectMask = parentCuboids.indicesOfIntersectingCuboids(cuboid);
		if(!intersectMask.any())
			return;
		// Fragment all intersected leaves.
		SmallSet<Cuboid> fragments;
		const auto& leafEnd = parent.getLeafCount();
		const auto& childBegin = parent.offsetOfFirstChild().get();
		BitSet intersectBitSet = BitSet::create(intersectMask);
		if(leafEnd != 0)
		{
			// Node has leaves.
			BitSet leafIntersectBitSet = intersectBitSet;
			if(childBegin != nodeSize)
				leafIntersectBitSet.clearAllAfterInclusive(childBegin);
			if(leafIntersectBitSet.any())
			{
				// Copy leafIntersectBitSet to send to eraseLeavesByMask.
				// TODO: combine two iterations.
				BitSet leafIntersectBitSet2 = leafIntersectBitSet;
				while(leafIntersectBitSet.any())
				{
					RTreeArrayIndex arrayIndex{leafIntersectBitSet.getNextAndClear()};
					fragments.insertAll(parentCuboids[arrayIndex.get()].getChildrenWhenSplitByCuboid(cuboid));
				}
				parent.eraseLeavesByMask(leafIntersectBitSet2);
			}
		}
		// insert fragments. Invalidates parent and cuboids, as well as index.
		const auto end = fragments.end();
		for(auto i = fragments.begin(); i < end; ++i)
			addToNodeRecursive(index2, *i);
	}
	// index2 remains valid. The value of "this" is no longer trustworthy due to potentially creating a new node.
	Node& parent = m_nodes[index2];
	if(parent.hasChildren())
	{
		BitSet intersectBitSet = BitSet::create(parent.getCuboids().indicesOfIntersectingCuboids(cuboid));
		intersectBitSet.clearAllBefore(parent.getLeafCount());
		addIntersectedChildrenToOpenList(parent, intersectBitSet, openList);
	}
	m_toComb.maybeInsert(index2);
}
void RTreeBoolean::merge(const RTreeNodeIndex& destination, const RTreeNodeIndex& source)
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
		for(const RTreeNodeIndex& index : copy)
		{
			Node& node = m_nodes[index];
			tryToMergeLeaves(node);
			// Try to merge child branches into this branch.
			bool merged = false;
			// plus one because we are removing one as well as adding.
			int capacity = node.unusedCapacity() + 1;
			const auto& nodeChildIndices = node.getChildIndices();
			for(RTreeArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			{
				// Take copy rather then reference because the original may be erased.
				const RTreeNodeIndex childIndex = nodeChildIndices[i];
				Node& childNode = m_nodes[childIndex];
				int childSize = childNode.getChildCount() + childNode.getLeafCount();
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
				const RTreeNodeIndex& parentIndex = node.getParent();
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
		const RTreeNodeIndex last = RTreeNodeIndex::create(m_nodes.size() - 1);
		auto found = m_emptySlots.find(last);
		if(found != m_emptySlots.end())
		{
			// Last node in m_nodes is in m_emptySlots.
			m_nodes.popBack();
			m_emptySlots.erase(found);
			continue;
		}
		const RTreeNodeIndex parentIndex = m_nodes[last].getParent();
		// Copy over the last slot in m_emptySlots.
		const RTreeNodeIndex empty = m_emptySlots.back();
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
			const RTreeNodeIndex& childIndex = newNodeChildren[i];
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
	StrongVector<RTreeNodeIndex, RTreeNodeIndex> indices;
	StrongVector<Node, RTreeNodeIndex> sortedNodes;
	indices.resize(m_nodes.size());
	sortedNodes.resize(m_nodes.size());
	std::iota(indices.begin(), indices.end(), RTreeNodeIndex::create(0));
	// Don't sort the first element, it is always root.
	std::ranges::sort(indices.begin() + 1, indices.end(), std::less{}, [&](const RTreeNodeIndex& index) { return m_nodes[index].sortOrder(); });
	sortedNodes[RTreeNodeIndex::create(0)] = m_nodes.front();
	const auto end = m_nodes.size();
	// Copy nodes into new order.
	for(RTreeNodeIndex oldIndex = RTreeNodeIndex::create(1); oldIndex < end; ++oldIndex)
	{
		const RTreeNodeIndex& newIndex = indices[oldIndex];
		assert(newIndex != 0);
		sortedNodes[newIndex] = m_nodes[oldIndex];
	}
	// Update copied nodes' parents and children.
	for(RTreeNodeIndex oldIndex = RTreeNodeIndex::create(1); oldIndex < end; ++oldIndex)
	{
		const RTreeNodeIndex& newIndex = indices[oldIndex];
		if(newIndex == oldIndex)
			continue;
		Node& newNode = sortedNodes[newIndex];
		sortedNodes[newNode.getParent()].updateChildIndex(oldIndex, newIndex);
		const auto& newNodeChildren = newNode.getChildIndices();
		for(RTreeArrayIndex i = newNode.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const RTreeNodeIndex& childIndex = newNodeChildren[i];
			assert(sortedNodes[childIndex].getParent() == oldIndex);
			sortedNodes[childIndex].setParent(newIndex);
		}
	}
	m_nodes = std::move(sortedNodes);
}
void RTreeBoolean::addIntersectedChildrenToOpenList(const Node& node, BitSet& intersectMask, SmallSet<RTreeNodeIndex>& openList)
{
	[[maybe_unused]] const auto offsetOfFirstChild = node.offsetOfFirstChild();
	// Any bits representing leaves have already been cleared.
	assert(intersectMask.getNext() >= offsetOfFirstChild.get());
	const auto& nodeChildren = node.getChildIndices();
	while(!intersectMask.empty())
	{
		const RTreeArrayIndex index{intersectMask.getNextAndClear()};
		openList.insert(nodeChildren[index]);
	}
}
void RTreeBoolean::maybeInsert(const Cuboid& cuboid)
{
	constexpr RTreeNodeIndex zeroIndex = RTreeNodeIndex::create(0);
	addToNodeRecursive(zeroIndex, cuboid);
}
void RTreeBoolean::maybeRemove(const Cuboid& cuboid)
{
	// Erase all contained branches and leaves.
	constexpr RTreeNodeIndex rootIndex = RTreeNodeIndex::create(0);
	clearAllContained(rootIndex, cuboid);
	SmallSet<RTreeNodeIndex> openList;
	SmallSet<RTreeNodeIndex> toUpdateBoundryMaybe;
	openList.insert(rootIndex);
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		removeFromNode(index, cuboid, openList);
		if(index != 0)
			toUpdateBoundryMaybe.maybeInsert(index);
	}
	for(const RTreeNodeIndex& index : toUpdateBoundryMaybe)
	{
		Node& node = m_nodes[index];
		const RTreeNodeIndex& parentIndex = node.getParent();
		Node& parent = m_nodes[parentIndex];
		const RTreeArrayIndex offset = parent.offsetFor(index);
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
bool RTreeBoolean::canPrepare() const
{
	return !m_toComb.empty() || !m_emptySlots.empty();
}
bool RTreeBoolean::query(const Point3D& begin, const Point3D& end) const { return query(ParamaterizedLine(begin, end)); }
CuboidSet RTreeBoolean::toCuboidSet() const
{
	CuboidSet output;
	for(RTreeNodeIndex i = RTreeNodeIndex::create(0); i < m_nodes.size(); ++i)
	{
		if(m_emptySlots.contains(i))
			continue;
		const Node& node = m_nodes[i];
		output.addSet(node.getLeafCuboids());
	}
	return output;
}
int RTreeBoolean::leafCount() const
{
	int output = 0;
	for(RTreeNodeIndex i = RTreeNodeIndex::create(0); i < m_nodes.size(); ++i)
	{
		if(m_emptySlots.contains(i))
			continue;
		const Node& node = m_nodes[i];
		output += node.getLeafCount();
	}
	return output;
}
const RTreeBoolean::Node& RTreeBoolean::getNode(int i) const { return m_nodes[RTreeNodeIndex::create(i)]; }
const Cuboid RTreeBoolean::getNodeCuboid(int i, int o) const { return m_nodes[RTreeNodeIndex::create(i)].getCuboids()[o]; }
const RTreeNodeIndex& RTreeBoolean::getNodeChild(int i, int o) const { return m_nodes[RTreeNodeIndex::create(i)].getChildIndices()[RTreeArrayIndex::create(o)]; }
bool RTreeBoolean::queryPoint(int x, int y, int z) const { return query(Point3D::create(x,y,z)); }
int RTreeBoolean::totalNodeVolume() const
{
	int output = 0;
	const RTreeNodeIndex end{m_nodes.size()};
	for(RTreeNodeIndex index = RTreeNodeIndex::create(0); index != end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getNodeVolume();
	return output;
}
int RTreeBoolean::totalLeafVolume() const
{
	int output = 0;
	const RTreeNodeIndex end{m_nodes.size()};
	for(RTreeNodeIndex index = RTreeNodeIndex::create(0); index != end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getLeafVolume();
	return output;
}
int RTreeBoolean::getNodeSize() { return nodeSize; }
void RTreeBoolean::assertAllLeafsAreUnique() const
{
	// Will assert fail if any leaves are not unique.
	[[maybe_unused]] const auto leaves = gatherLeavesRecursive(RTreeNodeIndex::create(0));
}
// Template methods.
template<typename ShapeT>
bool RTreeBoolean::query(const ShapeT& shape) const
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(RTreeNodeIndex::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		auto intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shape);
		const auto leafCount = node.getLeafCount();
		if(leafCount != 0 && intersectMask.head(leafCount).any())
			return true;
		const auto childCount = node.getChildCount();
		if(node.hasChildren() && intersectMask.tail(childCount).any())
		{
			auto bitset = BitSet::create(intersectMask);
			addIntersectedChildrenToOpenList(node, bitset, openList);
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
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(RTreeNodeIndex::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		BitSet intersectMask = BitSet::create(nodeCuboids.indicesOfIntersectingCuboids(shape));
		if(intersectMask.any())
		{
			const RTreeArrayIndex arrayIndex{intersectMask.getNext()};
			if(arrayIndex < node.getLeafCount())
				return nodeCuboids[arrayIndex.get()];
			addIntersectedChildrenToOpenList(node, intersectMask, openList);
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
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(RTreeNodeIndex::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		BitSet intersectMask = BitSet::create(nodeCuboids.indicesOfIntersectingCuboids(shape));
		if(!intersectMask.empty())
		{
			const RTreeArrayIndex arrayIndex{intersectMask.getNext()};
			if(arrayIndex < node.getLeafCount())
				return node.getCuboids()[arrayIndex.get()].intersectionPoint(shape);
			addIntersectedChildrenToOpenList(node, intersectMask, openList);
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
	SmallSet<int> topLevelCandidates;
	topLevelCandidates.resize(shapes.size());
	std::iota(topLevelCandidates.m_data.begin(), topLevelCandidates.m_data.end(), 0);
	SmallMap<RTreeNodeIndex, SmallSet<int>> openlist;
	openlist.insert(RTreeNodeIndex::create(0), std::move(topLevelCandidates));
	while(!openlist.empty())
	{
		const auto& [index, candidates] = openlist.back();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		const auto& nodeChildren = node.getChildIndices();
		const auto offsetOfFirstChild = node.offsetOfFirstChild();
		for(const int& shapeIndex : candidates)
		{
			if(output[shapeIndex])
				// this shape has already intersected with a leaf, no need to check further.
				continue;
			const auto& intersectMask = nodeCuboids.indicesOfIntersectingCuboids(shapes[shapeIndex]);
			// record all leaf intersections.
			const auto leafCount = node.getLeafCount();
			if(leafCount != 0 && intersectMask.head(leafCount).any())
			{
				output[shapeIndex] = true;
				continue;
			}
			// record all node intersections.
			BitSet intersectBitSet = BitSet::create(intersectMask);
			intersectBitSet.clearAllBefore(offsetOfFirstChild.get());
			while(!intersectBitSet.empty())
			{
				const RTreeArrayIndex arrayIndex{intersectBitSet.getNext()};
				openlist.getOrCreate(nodeChildren[arrayIndex]).insert(shapeIndex);
			}
		}
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
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(RTreeNodeIndex::create(0));
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		const Node& node = m_nodes[index];
		const auto& nodeCuboids = node.getCuboids();
		BitSet intersectMask = BitSet::create(nodeCuboids.indicesOfIntersectingCuboids(shape));
		BitSet leafIntersectMask = intersectMask;
		const auto& leafCount = node.getLeafCount();
		if(leafCount != 0)
		{
			// Leaves exist.
			if(leafCount != nodeSize)
				// Clear any child indices that may exist in leaf intercept mask.
				leafIntersectMask.clearAllAfterInclusive(leafCount);
			while(!leafIntersectMask.empty())
			{
				const RTreeArrayIndex arrayIndex{leafIntersectMask.getNextAndClear()};
				output.maybeAdd(nodeCuboids[arrayIndex.get()]);
			}
		}
		const auto& offsetOfFirstChild = node.offsetOfFirstChild();
		if(offsetOfFirstChild.get() != nodeSize)
		{
			// Children exist.
			intersectMask.clearAllBefore(offsetOfFirstChild.get());
			addIntersectedChildrenToOpenList(node, intersectMask, openList);
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
				output.maybeAdd(leaf.intersection(subShape));
	}
	else
		for(const Cuboid& leaf : queryGetLeaves(shape))
			output.maybeAdd(leaf.intersection(shape));
	return output;
}
template CuboidSet RTreeBoolean::queryGetIntersection(const Cuboid& shape) const;
template CuboidSet RTreeBoolean::queryGetIntersection(const CuboidSet& shape) const;
void RTreeBoolean::queryRemove(CuboidSet& set) const
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(RTreeNodeIndex::create(0));
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
			const RTreeArrayIndex arrayIndex{interceptMask.getNextAndClear()};
			if(arrayIndex >= leafCount)
				break;
			Cuboid cuboid = nodeCuboids[arrayIndex.get()];
			set.remove(cuboid);
		}
		if(!interceptMask.empty())
			addIntersectedChildrenToOpenList(node, interceptMask, openList);
	}
}