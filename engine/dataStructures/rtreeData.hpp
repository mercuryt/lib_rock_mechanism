#include "rtreeData.h"
template<typename T>
RTreeData<T>::ArrayIndex RTreeData<T>::Node::offsetFor(const Index& index) const
{
	auto found = std::ranges::find_if(m_dataAndChildIndices.begin() + m_childBegin.get(), m_dataAndChildIndices.end(), [&](const DataOrChild& dataOrChild) { return dataOrChild.child == index.get(); });
	assert(found != m_dataAndChildIndices.end());
	return ArrayIndex::create(std::distance(m_dataAndChildIndices.begin(), found));
}
template<typename T>
uint RTreeData<T>::Node::getLeafVolume() const
{
	uint output = 0;
	for(auto i = ArrayIndex::create(0); i < m_leafEnd; ++i)
		output += m_cuboids[i.get()].size();
	return output;
}
template<typename T>
uint RTreeData<T>::Node::getNodeVolume() const
{
	uint output = 0;
	for(auto i = m_childBegin; i < nodeSize; ++i)
		output += m_cuboids[i.get()].size();
	return output;
}
template<typename T>
void RTreeData<T>::Node::updateChildIndex(const Index& oldIndex, const Index& newIndex)
{
	ArrayIndex offset = offsetFor(oldIndex);
	m_dataAndChildIndices[offset.get()].child = newIndex.get();
}
template<typename T>
void RTreeData<T>::Node::insertLeaf(const Cuboid& cuboid, const T& value)
{
	assert(m_leafEnd != m_childBegin);
	m_cuboids.insert(m_leafEnd.get(), cuboid);
	m_dataAndChildIndices[m_leafEnd.get()].data = value.get();
	++m_leafEnd;
}
template<typename T>
void RTreeData<T>::Node::insertBranch(const Cuboid& cuboid, const Index& index)
{
	assert(m_leafEnd != m_childBegin);
	uint toInsertAt = m_childBegin.get() - 1;
	m_dataAndChildIndices[toInsertAt].child = index.get();
	m_cuboids.insert(toInsertAt, cuboid);
	--m_childBegin;
}
template<typename T>
void RTreeData<T>::Node::eraseBranch(const ArrayIndex& offset)
{
	assert(offset < nodeSize);
	assert(offset >= m_childBegin);
	if(offset != m_childBegin)
	{
		m_cuboids.insert(offset.get(), m_cuboids[m_childBegin.get()]);
		m_dataAndChildIndices[offset.get()].child = m_dataAndChildIndices[m_childBegin.get()].child;
	}
	m_cuboids.erase(m_childBegin.get());
	m_dataAndChildIndices[m_childBegin.get()].child = Index::null().get();
	++m_childBegin;
}
template<typename T>
void RTreeData<T>::Node::eraseLeaf(const ArrayIndex& offset)
{
	assert(offset < m_leafEnd);
	ArrayIndex lastLeaf = m_leafEnd - 1;
	if(offset != lastLeaf)
	{
		m_cuboids.insert(offset.get(), m_cuboids[lastLeaf.get()]);
		m_dataAndChildIndices[offset.get()].data = m_dataAndChildIndices[lastLeaf.get()].data;
	}
	m_cuboids.erase(lastLeaf.get());
	m_dataAndChildIndices[lastLeaf.get()].data = T::null().get();
	--m_leafEnd;
}
template<typename T>
void RTreeData<T>::Node::eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
{
	ArrayIndex leafEnd = m_leafEnd;
	ArrayIndex childBegin = m_childBegin;
	const auto copyCuboids = m_cuboids;
	const auto copyChildren = m_dataAndChildIndices;
	clear();
	for(ArrayIndex i = ArrayIndex::create(0); i < leafEnd; ++i)
		if(!mask[i.get()])
			insertLeaf(copyCuboids[i.get()], T::create(copyChildren[i.get()].data));
	for(ArrayIndex i = childBegin; i < nodeSize; ++i)
		if(!mask[i.get()])
			insertBranch(copyCuboids[i.get()], Index::create(copyChildren[i.get()].child));
}
template<typename T>
void RTreeData<T>::Node::clear()
{
	m_cuboids.clear();
	for(ArrayIndex i = ArrayIndex::create(0); i < m_leafEnd; ++i)
		m_dataAndChildIndices[i.get()].data = T::null().get();
	for(ArrayIndex i = m_childBegin; i < nodeSize; ++i)
		m_dataAndChildIndices[i.get()].child = Index::null().get();
	m_leafEnd = ArrayIndex::create(0);
	m_childBegin = ArrayIndex::create(nodeSize);
}
template<typename T>
void RTreeData<T>::Node::updateLeaf(const ArrayIndex offset, const Cuboid& cuboid)
{
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
template<typename T>
void RTreeData<T>::Node::updateLeafWithValue(const ArrayIndex offset, const Cuboid& cuboid, const T& value)
{
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
	m_dataAndChildIndices[offset.get()].data = value.get();
}
template<typename T>
void RTreeData<T>::Node::updateBranchBoundry(const ArrayIndex offset, const Cuboid& cuboid)
{
	assert(offset >= m_childBegin);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
template<typename T>
std::string RTreeData<T>::Node::toString()
{
	std::string output = "parent: " + m_parent.toString() + "; ";
	if(m_leafEnd != 0)
	{
		output += "leaves: {";
		for(ArrayIndex i = ArrayIndex::create(0); i < m_leafEnd; ++i)
			output += "(" + m_cuboids[i.get()].toString()  + ":" + std::to_string(m_dataAndChildIndices[i.get()].data) + "), ";
		output += "}; ";
	}
	if(m_childBegin != nodeSize)
	{
		output += "children: {";
		for(ArrayIndex i = m_childBegin; i < nodeSize; ++i)
			output += "(" + m_cuboids[i.get()].toString() + ":" + std::to_string(m_dataAndChildIndices[i.get()].child) + "), ";
		output += "}; ";
	}
	return output;
}
template<typename T>
std::tuple<Cuboid, typename RTreeData<T>::ArrayIndex, typename RTreeData<T>::ArrayIndex> RTreeData<T>::findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const
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
			const uint size = sum.size();
			int emptySpace = size - (firstCuboid.size() + secondCuboid.size());
			// If empty space is equal then prefer the smaller result.
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
template<typename T>
SmallMap<Cuboid, T> RTreeData<T>::gatherLeavesRecursive(const Index& parent) const
{
	SmallMap<Cuboid, T> output;
	SmallSet<Index> openList;
	openList.insert(parent);
	while(!openList.empty())
	{
		const Node& node = m_nodes[openList.back()];
		openList.popBack();
		const auto endLeaf = node.getLeafCount();
		const auto& nodeCuboids = node.getCuboids();
		const auto& dataAndChildren = node.getDataAndChildIndices();
		for(int i = 0; i != endLeaf; ++i)
			output.insert(nodeCuboids[i], T::create(dataAndChildren[i].data));
		for(ArrayIndex i = node.offsetOfFirstChild(); i != nodeSize; ++i)
			openList.insert(Index::create(dataAndChildren[i.get()].child));
	}
	return output;
}
template<typename T>
void RTreeData<T>::destroyWithChildren(const Index& index)
{
	SmallSet<Index> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		Index index = openList.back();
		m_emptySlots.insert(index);
		Node& node = m_nodes[index];
		const auto& dataAndChildren = node.getDataAndChildIndices();
		openList.popBack();
		for(auto i = node.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const auto& childIndex = Index::create(dataAndChildren[i.get()].child);
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
}
template<typename T>
void RTreeData<T>::tryToMergeLeaves(Node& parent)
{
	// Iterate through leaves
	ArrayIndex offset = ArrayIndex::create(0);
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
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
			if(mergeMask[i.get()] && parentDataAndChildIndices[i.get()].data == parentDataAndChildIndices[offset.get()].data)
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
template<typename T>
void RTreeData<T>::clearAllContained(Node& parent, const Cuboid& cuboid)
{
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
	const auto containsMask = parentCuboids.indicesOfContainedCuboids(cuboid);
	if(!containsMask.any())
		return;
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
		if(containsMask[i.get()])
			destroyWithChildren(Index::create(parentDataAndChildIndices[i.get()].child));
	parent.eraseByMask(containsMask);
}
template<typename T>
void RTreeData<T>::clearAllContainedWithValueRecursive(Node& parent, const Cuboid& cuboid, const T& value)
{
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
	auto containsMask = parentCuboids.indicesOfContainedCuboids(cuboid);
	const auto leafCount = parent.getLeafCount();
	if(!containsMask.any())
		return;
	for(ArrayIndex i = ArrayIndex::create(0); i < leafCount; ++i)
		if(parentDataAndChildIndices[i.get()].data != value.get())
			containsMask[i.get()] = false;
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
	{
		const Index childIndex = Index::create(parentDataAndChildIndices[i.get()].child);
		Node& child = m_nodes[childIndex];
		clearAllContainedWithValueRecursive(child, cuboid, value);
		m_toComb.maybeInsert(childIndex);
		if(!child.empty())
			containsMask[i.get()] = false;
	}
	parent.eraseByMask(containsMask);
}
template<typename T>
void RTreeData<T>::addToNodeRecursive(const Index& index, const Cuboid& cuboid, const T& value)
{
	m_toComb.maybeInsert(index);
	Node& parent = m_nodes[index];
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
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
		const bool secondArrayIndexIsNewCuboid = secondArrayIndex == nodeSize;
		if(firstArrayIndex < leafCount)
		{
			if(secondArrayIndex == nodeSize || secondArrayIndex < leafCount)
			{
				// Both first and second cuboids are leaves.
				if(
					(mergedCuboid.size() == extended[firstArrayIndex.get()].size() + extended[secondArrayIndex.get()].size()) &&
					(
						// Can merge if second index is the newly added cuboid and first index has the same value.
						(parentDataAndChildIndices[firstArrayIndex.get()].data == value.get() && secondArrayIndexIsNewCuboid) ||
						// Can merge if first and second array indices have the same value.
						(!secondArrayIndexIsNewCuboid && parentDataAndChildIndices[firstArrayIndex.get()].data == parentDataAndChildIndices[secondArrayIndex.get()].data)
					)
				)
				{
					// Can merge into a single leaf.
					parent.updateLeaf(firstArrayIndex, mergedCuboid);
					if(secondArrayIndex != nodeSize)
						// The newly inserted cuboid was not merged. Store it in now avalible second offset, overwriting the leaf which was merged.
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
					newNode.insertLeaf(extended[firstArrayIndex.get()], T::create(parentDataAndChildIndices[firstArrayIndex.get()].data));
					const T& valueToInsertForSecondIndex = secondArrayIndexIsNewCuboid ? value : T::create(parentDataAndChildIndices[secondArrayIndex.get()].data);
					newNode.insertLeaf(extended[secondArrayIndex.get()], valueToInsertForSecondIndex);
					Node& parent2 = m_nodes[indexCopy];
					if(!secondArrayIndexIsNewCuboid)
						// The newly inserted cuboid was not merged into newNode. Store it in parent.
						parent2.updateLeafWithValue(secondArrayIndex, cuboid, value);
					parent2.eraseLeaf(firstArrayIndex);
					parent2.insertBranch(mergedCuboid, newIndex);

				}
			}
			else
			{
				// Update the boundry for the branch.
				parent.updateBranchBoundry(secondArrayIndex, mergedCuboid);
				// Store the new cuboid in the now avalible first offset, overwriting the leaf which was merged.
				parent.updateLeafWithValue(firstArrayIndex, cuboid, value);
				// Merge leaf at first offset location with branch at second.
				// Invalidates parent and array indices.
				addToNodeRecursive(
					Index::create(parentDataAndChildIndices[secondArrayIndex.get()].child),
					extended[firstArrayIndex.get()],
					T::create(parentDataAndChildIndices[firstArrayIndex.get()].data)
				);
			}
		}
		else
		{
			if(secondArrayIndex == nodeSize || secondArrayIndex < leafCount)
			{
				// Update the boundry for the branch.
				parent.updateBranchBoundry(firstArrayIndex, mergedCuboid);
				if(secondArrayIndex != nodeSize)
					// The newly inserted cuboid was not merged. Store it in now avalible second offset, overwriting the leaf which was merged.
					parent.updateLeafWithValue(secondArrayIndex, cuboid, value);
				// Merge leaf at second offset with branch at first.
				// Invalidates parent and array indices.
				addToNodeRecursive(
					Index::create(parentDataAndChildIndices[firstArrayIndex.get()].child),
					extended[secondArrayIndex.get()],
					T::create(parentDataAndChildIndices[secondArrayIndex.get()].data)
				);
			}
			else
			{
				// Both first and second offsets are branches, merge second into first.
				const Index firstIndex = Index::create(parentDataAndChildIndices[firstArrayIndex.get()].child);
				const Index secondIndex = Index::create(parentDataAndChildIndices[secondArrayIndex.get()].child);
				const Index indexCopy = index;
				// Invalidates parent and index.
				merge(firstIndex, secondIndex);
				Node& parent2 = m_nodes[indexCopy];
				// Update the boundry for the first branch.
				parent2.updateBranchBoundry(firstArrayIndex, mergedCuboid);
				// Erase the second branch and insert the leaf.
				parent2.eraseBranch(secondArrayIndex);
				// Mark second branch node and all children for removal.
				destroyWithChildren(secondIndex);
				// Use now open slot to store new leaf.
				parent2.insertLeaf(cuboid, value);
			}
		}
	}
	else
		parent.insertLeaf(cuboid, value);
}
template<typename T>
void RTreeData<T>::removeFromNode(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList)
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
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
		leafInterceptMask[i.get()] = 0;
	// erase all intercepted leaves.
	parent.eraseByMask(leafInterceptMask);
	const Index index2 = index;
	// insert fragments. Invalidates parent and cuboids, as well as index.
	const auto end = fragments.end();
	for(auto i = fragments.begin(); i < end; ++i)
		addToNodeRecursive(index, *i);
	Node& parent2 = m_nodes[index2];
	const auto& parentDataAndChildIndices = parent2.getDataAndChildIndices();
	// Erasing and adding leaves has not invalidated intercept mask for branches.
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
	{
		if(interceptMask[i.get()])
		{
			const Index& childIndex = Index::create(parentDataAndChildIndices[i.get()].child);
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
	m_toComb.maybeInsert(index);
}
template<typename T>
void RTreeData<T>::removeFromNodeWithValue(const Index& index, const Cuboid& cuboid, SmallSet<Index>& openList, const T& value)
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
	for(ArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
		leafInterceptMask[i.get()] = 0;
	const auto leafCount = parent.getLeafCount();
	// Exclude leafs with different value.
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
	for(ArrayIndex i = ArrayIndex::create(0); i < leafCount; ++i)
		if(parentDataAndChildIndices[i.get()].data != value.get())
			leafInterceptMask[i.get()] = 0;
	// erase all intercepted leaves.
	parent.eraseByMask(leafInterceptMask);
	const Index index2 = index;
	// insert fragments. Invalidates parent and cuboids, as well as index.
	const auto end = fragments.end();
	for(auto i = fragments.begin(); i < end; ++i)
		addToNodeRecursive(index, *i, value);
	Node& parent2 = m_nodes[index2];
	const auto& parentDataAndChildIndices2 = parent2.getDataAndChildIndices();
	// Erasing and adding leaves has not invalidated intercept mask for branches.
	for(ArrayIndex i = parent2.offsetOfFirstChild(); i < nodeSize; ++i)
	{
		if(interceptMask[i.get()])
		{
			const Index& childIndex = Index::create(parentDataAndChildIndices2[i.get()].child);
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
	m_toComb.maybeInsert(index);
}
template<typename T>
void RTreeData<T>::merge(const Index& destination, const Index& source)
{
	// TODO: maybe leave source nodes intact instead of striping out leaves?
	const auto leaves = gatherLeavesRecursive(source);
	for(const auto& [leaf, value] : leaves)
		addToNodeRecursive(destination, leaf, value);
}
// This method is identical to the one in rTreeBoolean, other then calling a different tryToMergeLeaves
template<typename T>
void RTreeData<T>::comb()
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
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			for(ArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			{
				// Take copy rather then reference because the original may be erased.
				const Index childIndex = Index::create(nodeDataAndChildIndices[i.get()].child);
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
// This method is identical to the one in rTreeBoolean.
template<typename T>
void RTreeData<T>::defragment()
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

		const auto& newNodeChildren = newNode.getDataAndChildIndices();
		for(auto i = newNode.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const Index& childIndex = Index::create(newNodeChildren[i.get()].child);
			if(childIndex.exists())
			{
				assert(m_nodes[childIndex].getParent() == last);
				m_nodes[childIndex].setParent(empty);
			}
		}
	}
}
// This method is identical to the one in rTreeBoolean.
template<typename T>
void RTreeData<T>::sort()
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
		const auto& newNodeChildren = newNode.getDataAndChildIndices();
		for(ArrayIndex i = newNode.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const Index& childIndex = Index::create(newNodeChildren[i.get()].child);
			assert(sortedNodes[childIndex].getParent() == oldIndex);
			sortedNodes[childIndex].setParent(newIndex);
		}
	}
	m_nodes = std::move(sortedNodes);
}
template<typename T>
void RTreeData<T>::maybeInsert(const Cuboid& cuboid, const T& value)
{
	constexpr Index zeroIndex = Index::create(0);
	addToNodeRecursive(zeroIndex, cuboid, value);
}
template<typename T>
void RTreeData<T>::maybeRemove(const Cuboid& cuboid, const T& value)
{
	// Erase all contained branches and leaves.
	constexpr Index rootIndex = Index::create(0);
	clearAllContainedWithValueRecursive(m_nodes[rootIndex], cuboid, value);
	SmallSet<Index> openList;
	SmallSet<Index> toUpdateBoundryMaybe;
	openList.insert(rootIndex);
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		removeFromNodeWithValue(index, cuboid, openList, value);
		if(index != 0)
			toUpdateBoundryMaybe.maybeInsert(index);
	}
	for(const Index& index : toUpdateBoundryMaybe)
	{
		Node& node = m_nodes[index];
		const Index parentIndex = node.getParent();
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
template<typename T>
void RTreeData<T>::prepare()
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
template<typename T>
uint RTreeData<T>::leafCount() const
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
template<typename T>
const RTreeData<T>::Node& RTreeData<T>::getNode(uint i) const { return m_nodes[Index::create(i)]; }
template<typename T>
const Cuboid RTreeData<T>::getNodeCuboid(uint i, uint o) const { return m_nodes[Index::create(i)].getCuboids()[o]; }
template<typename T>
const RTreeData<T>::Index& RTreeData<T>::getNodeChild(uint i, uint o) const { return Index::create(m_nodes[Index::create(i)].getDataAndChildIndices()[o].child); }
template<typename T>
bool RTreeData<T>::queryPoint(uint x, uint y, uint z) const { return queryAny(Point3D::create(x,y,z)); }
template<typename T>
uint RTreeData<T>::totalNodeVolume() const
{
	uint output = 0;
	const auto end = m_nodes.size();
	for(Index index = Index::create(0); index < end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getNodeVolume();
	return output;
}
template<typename T>
uint RTreeData<T>::totalLeafVolume() const
{
	uint output = 0;
	const auto end = m_nodes.size();
	for(Index index = Index::create(0); index < end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getLeafVolume();
	return output;
}
template<typename T>
uint RTreeData<T>::getNodeSize() { return nodeSize; }
