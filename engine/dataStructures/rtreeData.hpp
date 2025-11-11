#pragma once
#include "rtreeData.h"
#include "../geometry/mapWithCuboidKeys.hpp"
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
RTreeArrayIndex RTreeData<T, config, nullPrimitive>::Node::offsetFor(const RTreeNodeIndex& index) const
{
	const auto begin = m_dataAndChildIndices.begin();
	auto found = std::ranges::find(begin + m_childBegin.get(), m_dataAndChildIndices.end(), index.get(), &DataOrChild::child);
	assert(found != m_dataAndChildIndices.end());
	const auto output = RTreeArrayIndex::create(found - begin);
	assert(output >= m_childBegin);
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint RTreeData<T, config, nullPrimitive>::Node::getLeafVolume() const
{
	uint output = 0;
	for(auto i = RTreeArrayIndex::create(0); i < m_leafEnd; ++i)
		output += m_cuboids[i.get()].volume();
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint RTreeData<T, config, nullPrimitive>::Node::getNodeVolume() const
{
	uint output = 0;
	for(auto i = m_childBegin; i < nodeSize; ++i)
		output += m_cuboids[i.get()].volume();
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
Json RTreeData<T, config, nullPrimitive>::Node::toJson() const
{
	if constexpr(HasToJsonMethod<T>)
	{
		Json output{
			{"cuboids", m_cuboids},
			{"dataAndChildIndices", Json::array()},
			{"parent", m_parent},
			{"leafEnd", m_leafEnd},
			{"childBegin", m_childBegin}
		};
		for(uint i = 0; i < m_leafEnd; ++i)
			output["dataAndChildIndices"][i] = m_dataAndChildIndices[i].data;
		for(uint i = m_childBegin.get(); i < nodeSize; ++i)
			output["dataAndChildIndices"][i] = m_dataAndChildIndices[i].child;
		return output;
	}
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
bool RTreeData<T, config, nullPrimitive>::Node::containsLeaf(const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	uint index = m_cuboids.indexOfCuboid(cuboid);
	if(index >= m_leafEnd.get())
		return false;
	return m_dataAndChildIndices[index].data == value.get();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::load(const Json& data)
{
	if constexpr(HasFromJsonMethod<T>)
	{
		data["cuboids"].get_to(m_cuboids);
		data["parent"].get_to(m_parent);
		data["leafEnd"].get_to(m_leafEnd);
		data["childBegin"].get_to(m_childBegin);
		for(uint i = 0; i < nodeSize; ++i)
		{
			if(i < m_leafEnd)
				data["dataAndChildIndices"][i].get_to(m_dataAndChildIndices[i].data);
			if(i >= m_childBegin)
				data["dataAndChildIndices"][i].get_to(m_dataAndChildIndices[i].child);
		}
	}
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::updateChildIndex(const RTreeNodeIndex& oldIndex, const RTreeNodeIndex& newIndex)
{
	RTreeArrayIndex offset = offsetFor(oldIndex);
	m_dataAndChildIndices[offset.get()].child = newIndex.get();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::insertLeaf(const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	assert(m_leafEnd != m_childBegin);
	assert(!containsLeaf(cuboid, value));
	m_cuboids.insert(m_leafEnd.get(), cuboid);
	m_dataAndChildIndices[m_leafEnd.get()].data = value.get();
	++m_leafEnd;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::insertBranch(const Cuboid& cuboid, const RTreeNodeIndex& index)
{
	assert(m_leafEnd != m_childBegin);
	uint toInsertAt = m_childBegin.get() - 1;
	m_dataAndChildIndices[toInsertAt].child = index.get();
	m_cuboids.insert(toInsertAt, cuboid);
	--m_childBegin;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::eraseBranch(const RTreeArrayIndex& offset)
{
	assert(offset < nodeSize);
	assert(offset >= m_childBegin);
	if(offset != m_childBegin)
	{
		m_cuboids.insert(offset.get(), m_cuboids[m_childBegin.get()]);
		m_dataAndChildIndices[offset.get()].child = m_dataAndChildIndices[m_childBegin.get()].child;
	}
	m_cuboids.erase(m_childBegin.get());
	m_dataAndChildIndices[m_childBegin.get()].child = RTreeNodeIndex::null().get();
	++m_childBegin;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::eraseLeaf(const RTreeArrayIndex& offset)
{
	assert(offset < m_leafEnd);
	const RTreeArrayIndex lastLeaf = m_leafEnd - 1;
	if(offset != lastLeaf)
	{
		m_cuboids.insert(offset.get(), m_cuboids[lastLeaf.get()]);
		m_dataAndChildIndices[offset.get()].data = m_dataAndChildIndices[lastLeaf.get()].data;
	}
	m_cuboids.erase(lastLeaf.get());
	m_dataAndChildIndices[lastLeaf.get()].data = T::null().get();
	--m_leafEnd;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::eraseByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
{
	const RTreeArrayIndex leafEnd = m_leafEnd;
	const RTreeArrayIndex childBegin = m_childBegin;
	const auto copyCuboids = m_cuboids;
	const auto copyChildren = m_dataAndChildIndices;
	clear();
	for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafEnd; ++i)
		if(!mask[i.get()])
			insertLeaf(copyCuboids[i.get()], T::create(copyChildren[i.get()].data));
	for(RTreeArrayIndex i = childBegin; i < nodeSize; ++i)
		if(!mask[i.get()])
			insertBranch(copyCuboids[i.get()], RTreeNodeIndex::create(copyChildren[i.get()].child));
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::eraseLeavesByMask(const BitSet<uint64_t, nodeSize>& mask)
{
	RTreeArrayIndex leafEnd = m_leafEnd;
	const auto copyCuboids = m_cuboids;
	const auto copyDataAndChildIndices = m_dataAndChildIndices;
	// Overwrite from copy.
	m_leafEnd = RTreeArrayIndex::create(0);
	for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafEnd; ++i)
		if(!mask[i.get()])
			insertLeaf(copyCuboids[i.get()], T::create(copyDataAndChildIndices[i.get()].data));
	// Clear any remaning leaves.
	for(auto i = m_leafEnd; i < leafEnd; ++i)
	{
		m_cuboids.erase(i.get());
		m_dataAndChildIndices[i.get()].data = T::null().get();
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::eraseLeavesByMask(const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
{
	RTreeArrayIndex leafEnd = m_leafEnd;
	const auto copyCuboids = m_cuboids;
	const auto copyDataAndChildIndices = m_dataAndChildIndices;
	// Overwrite from copy.
	m_leafEnd = RTreeArrayIndex::create(0);
	for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafEnd; ++i)
		if(!mask[i.get()])
			insertLeaf(copyCuboids[i.get()], T::create(copyDataAndChildIndices[i.get()].data));
	// Clear any remaning leaves.
	for(auto i = m_leafEnd; i < leafEnd; ++i)
	{
		m_cuboids.erase(i.get());
		m_dataAndChildIndices[i.get()].data = T::null().get();
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::clear()
{
	m_cuboids.clear();
	for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < m_leafEnd; ++i)
		m_dataAndChildIndices[i.get()].data = T::null().get();
	for(RTreeArrayIndex i = m_childBegin; i < nodeSize; ++i)
		m_dataAndChildIndices[i.get()].child = RTreeNodeIndex::null().get();
	m_leafEnd = RTreeArrayIndex::create(0);
	m_childBegin = RTreeArrayIndex::create(nodeSize);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::updateLeaf(const RTreeArrayIndex& offset, const Cuboid& cuboid)
{
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	[[maybe_unused]] const auto value = T::create(m_dataAndChildIndices[offset.get()].data);
	assert(!containsLeaf(cuboid, value));
	m_cuboids.insert(offset.get(), cuboid);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::updateValue(const RTreeArrayIndex& offset, const T& value)
{
	assert(offset < m_leafEnd);
	assert(value != T::create(nullPrimitive));
	[[maybe_unused]] const auto cuboid = m_cuboids[offset.get()];
	assert(!cuboid.empty());
	assert(!containsLeaf(cuboid, value));
	m_dataAndChildIndices[offset.get()].data = value.get();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::updateLeafWithValue(const RTreeArrayIndex& offset, const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	assert(!containsLeaf(cuboid, value));
	m_cuboids.insert(offset.get(), cuboid);
	m_dataAndChildIndices[offset.get()].data = value.get();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::maybeUpdateLeafWithValue(const RTreeArrayIndex& offset, const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	assert(offset < m_leafEnd);
	assert(!m_cuboids[offset.get()].empty());
	if(m_cuboids[offset.get()] == cuboid && m_dataAndChildIndices[offset.get()].data == value.get())
		return;
	assert(!containsLeaf(cuboid, value));
	m_cuboids.insert(offset.get(), cuboid);
	m_dataAndChildIndices[offset.get()].data = value.get();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::Node::updateBranchBoundry(const RTreeArrayIndex& offset, const Cuboid& cuboid)
{
	assert(offset >= m_childBegin);
	assert(!m_cuboids[offset.get()].empty());
	m_cuboids.insert(offset.get(), cuboid);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
std::string RTreeData<T, config, nullPrimitive>::Node::toString()
{
	std::string output = "parent: " + m_parent.toString() + "; ";
	if(m_leafEnd != 0)
	{
		output += "leaves: {";
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < m_leafEnd; ++i)
			if constexpr(HasToStringMethod<T>)
				output += "(" + m_cuboids[i.get()].toString()  + ":" + T::create(m_dataAndChildIndices[i.get()].data).toString() + "), ";
			else if constexpr(std::is_pointer_v<typename T::Primitive>)
				output += "(" + m_cuboids[i.get()].toString()  + ":" + std::to_string((uintptr_t)m_dataAndChildIndices[i.get()].data) + "), ";
			else
				output += "(" + m_cuboids[i.get()].toString()  + ":" + std::to_string(m_dataAndChildIndices[i.get()].data) + "), ";
		output += "}; ";
	}
	if(m_childBegin != nodeSize)
	{
		output += "children: {";
		for(RTreeArrayIndex i = m_childBegin; i < nodeSize; ++i)
			output += "(" + m_cuboids[i.get()].toString() + ":" + std::to_string(m_dataAndChildIndices[i.get()].child) + "), ";
		output += "}; ";
	}
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
std::tuple<Cuboid, RTreeArrayIndex, RTreeArrayIndex> RTreeData<T, config, nullPrimitive>::findPairWithLeastNewVolumeWhenExtended(const CuboidArray<nodeSize + 1>& cuboids) const
{
	// May be negitive because resulting cuboids may intersect.
	int resultEmptySpace = INT32_MAX;
	uint resultSize = UINT32_MAX;
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
			const uint size = sum.volume();
			int emptySpace = size - (firstCuboid.volume() + secondCuboid.volume());
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
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
SmallSet<std::pair<Cuboid, T>> RTreeData<T, config, nullPrimitive>::gatherLeavesRecursive(const RTreeNodeIndex& parent) const
{
	SmallSet<std::pair<Cuboid, T>> output;
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(parent);
	while(!openList.empty())
	{
		const Node& node = m_nodes[openList.back()];
		openList.popBack();
		const auto endLeaf = node.getLeafCount();
		const auto& nodeCuboids = node.getCuboids();
		const auto& dataAndChildren = node.getDataAndChildIndices();
		for(int i = 0; i != endLeaf; ++i)
			output.emplace(nodeCuboids[i], T::create(dataAndChildren[i].data));
		for(RTreeArrayIndex i = node.offsetOfFirstChild(); i != nodeSize; ++i)
			openList.insert(RTreeNodeIndex::create(dataAndChildren[i.get()].child));
	}
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::destroyWithChildren(const RTreeNodeIndex& index)
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		RTreeNodeIndex current = openList.back();
		openList.popBack();
		m_emptySlots.insert(current);
		Node& node = m_nodes[current];
		const auto& dataAndChildren = node.getDataAndChildIndices();
		for(auto i = node.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const auto& childIndex = RTreeNodeIndex::create(dataAndChildren[i.get()].child);
			assert(childIndex.exists());
			openList.insert(childIndex);
		}
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::tryToMergeLeaves(Node& parent)
{
	// Iterate through leaves
	RTreeArrayIndex offset = RTreeArrayIndex::create(0);
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
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafCount; ++i)
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
			offset = RTreeArrayIndex::create(0);
		else
			// If no merge is made increment the offset
			++offset;
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::clearAllContained(const RTreeNodeIndex& index, const Cuboid& cuboid)
{
	SmallSet<RTreeNodeIndex> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		Node& node = m_nodes[openList.back()];
		openList.popBack();
		const auto& nodeCuboids = node.getCuboids();
		const auto containsMask = nodeCuboids.indicesOfContainedCuboids(cuboid);
		removeFromNodeByMask(node, containsMask);
		// Gather branches intercepted with to add to openList.
		const auto interceptMask = nodeCuboids.indicesOfIntersectingCuboids(cuboid);
		if(interceptMask.any())
		{
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			const auto begin = interceptMask.begin();
			const auto end = interceptMask.end();
			for(auto iter = begin + node.offsetOfFirstChild().get(); iter != end; ++iter)
				if(*iter)
				{
					RTreeArrayIndex iterIndex = RTreeArrayIndex::create(iter - begin);
					openList.insert(RTreeNodeIndex::create(nodeDataAndChildIndices[iterIndex.get()].child));
				}
		}
		m_toComb.maybeInsert(index);
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::clearAllContainedWithValueRecursive(Node& parent, const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
	auto containsMask = parentCuboids.indicesOfContainedCuboids(cuboid);
	const auto leafCount = parent.getLeafCount();
	if(!containsMask.any())
		return;
	for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafCount; ++i)
		if(parentDataAndChildIndices[i.get()].data != value.get())
			containsMask[i.get()] = false;
	for(RTreeArrayIndex i = parent.offsetOfFirstChild(); i < nodeSize; ++i)
	{
		const RTreeNodeIndex childIndex = RTreeNodeIndex::create(parentDataAndChildIndices[i.get()].child);
		Node& child = m_nodes[childIndex];
		clearAllContainedWithValueRecursive(child, cuboid, value);
		if(!child.empty())
		{
			m_toComb.maybeInsert(childIndex);
			containsMask[i.get()] = false;
		}
	}
	parent.eraseByMask(containsMask);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::addToNodeRecursive(const RTreeNodeIndex& index, const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	m_toComb.maybeInsert(index);
	Node& parent = m_nodes[index];
	const auto& parentCuboids = parent.getCuboids();
	const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
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
		const bool secondArrayIndexIsNewCuboid = secondArrayIndex == nodeSize;
		if(firstArrayIndex < leafCount)
		{
			if(secondArrayIndex == nodeSize || secondArrayIndex < leafCount)
			{
				// Both first and second cuboids are leaves.
				bool merge = false;
				if constexpr (config.splitAndMerge)
				{
					merge = (
						(mergedCuboid.volume() == extended[firstArrayIndex.get()].volume() + extended[secondArrayIndex.get()].volume()) &&
						(
							// Can merge if second index is the newly added cuboid and first index has the same value.
							(parentDataAndChildIndices[firstArrayIndex.get()].data == value.get() && secondArrayIndexIsNewCuboid) ||
							// Can merge if first and second array indices have the same value.
							(!secondArrayIndexIsNewCuboid && parentDataAndChildIndices[firstArrayIndex.get()].data == parentDataAndChildIndices[secondArrayIndex.get()].data)
						)
					);
				}
				if(merge)
				{
					// Can merge into a single leaf.
					parent.updateLeaf(firstArrayIndex, mergedCuboid);
					if(secondArrayIndex != nodeSize)
						// The newly inserted cuboid was not merged. Store it in now avalible second offset, overwriting the leaf which was merged.
						parent.updateLeafWithValue(secondArrayIndex, cuboid, value);
				}
				else
				{
					const RTreeNodeIndex indexCopy = index;
					// create a new node to hold first and second. Invalidates parent and index.
					m_nodes.add();
					Node& newNode = m_nodes.back();
					newNode.setParent(indexCopy);
					Node& parent2 = m_nodes[indexCopy];
					const auto& parentDataAndChildIndices2 = parent2.getDataAndChildIndices();
					const RTreeNodeIndex newIndex = RTreeNodeIndex::create(m_nodes.size() - 1);
					assert(!m_emptySlots.contains(newIndex));
					newNode.insertLeaf(extended[firstArrayIndex.get()], T::create(parentDataAndChildIndices2[firstArrayIndex.get()].data));
					const T& valueToInsertForSecondIndex = secondArrayIndexIsNewCuboid ? value : T::create(parentDataAndChildIndices2[secondArrayIndex.get()].data);
					newNode.insertLeaf(extended[secondArrayIndex.get()], valueToInsertForSecondIndex);
					if(!secondArrayIndexIsNewCuboid)
						// The newly inserted cuboid was not merged into newNode. Store it in parent.
						parent2.updateLeafWithValue(secondArrayIndex, cuboid, value);
					parent2.eraseLeaf(firstArrayIndex);
					parent2.insertBranch(mergedCuboid, newIndex);

				}
			}
			else
			{
				// Merge leaf at first offset location with branch at second.
				// Update the boundry for the branch.
				parent.updateBranchBoundry(secondArrayIndex, mergedCuboid);
				const RTreeNodeIndex index2 = index;
				// Invalidates parent and index.
				addToNodeRecursive(
					RTreeNodeIndex::create(parentDataAndChildIndices[secondArrayIndex.get()].child),
					extended[firstArrayIndex.get()],
					T::create(parentDataAndChildIndices[firstArrayIndex.get()].data)
				);
				Node& parent2 = m_nodes[index2];
				// Store the new cuboid in the now avalible first offset, overwriting the leaf which was merged.
				parent2.updateLeafWithValue(firstArrayIndex, cuboid, value);
			}
		}
		else
		{
			// First index is a branch. Second RTreeNodeIndex must either be another branch or the new leaf.
			if(secondArrayIndex == nodeSize)
			{
				RTreeNodeIndex index2 = index;
				// Add the new leaf to branch at first index.
				// Invalidates parent and index.
				addToNodeRecursive(
					RTreeNodeIndex::create(parentDataAndChildIndices[firstArrayIndex.get()].child),
					cuboid,
					value
				);
				Node& parent2 = m_nodes[index2];
				// Update the boundry for the branch.
				parent2.updateBranchBoundry(firstArrayIndex, mergedCuboid);
			}
			else
			{
				// Both first and second offsets are branches, merge second into first.
				const RTreeNodeIndex firstIndex = RTreeNodeIndex::create(parentDataAndChildIndices[firstArrayIndex.get()].child);
				const RTreeNodeIndex secondIndex = RTreeNodeIndex::create(parentDataAndChildIndices[secondArrayIndex.get()].child);
				const RTreeNodeIndex indexCopy = index;
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
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::removeFromNode(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList)
{
	CuboidArray<nodeSize>::BoolArray interceptMask;
	// Copy index before possibly invalidating it later.
	RTreeNodeIndex indexCopy = index;
	// These node references are going to be invalidated by inserting fragments, which may generate a new node.
	// A scope is used to ensure they are not used after they are invalidated.
	{
		Node& parent = m_nodes[indexCopy];
		const auto& parentCuboids = parent.getCuboids();
		const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
		interceptMask = parentCuboids.indicesOfIntersectingCuboids(cuboid);
		// Fragment all intercepted leaves.
		SmallSet<std::pair<Cuboid, T>> fragments;
		const uint8_t& leafCount = parent.getLeafCount();
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafCount; ++i)
			if(interceptMask[i.get()])
				for(const Cuboid& fragment : parentCuboids[i.get()].getChildrenWhenSplitByCuboid(cuboid))
					fragments.emplace(fragment, T::create(parentDataAndChildIndices[i.get()].data));
		// TODO: this could probably be better in regards to avoiding fragment generation rather then checking for empty. Will have to add more complex DEBUG logic.
		if constexpr(!config.splitAndMerge)
			assert(fragments.empty());
		// Copy intercept mask head to create leaf intercept mask.
		auto leafInterceptMask = interceptMask.head(leafCount);
		// erase all intercepted leaves.
		parent.eraseLeavesByMask(leafInterceptMask);
		// insert fragments. Invalidates parent and cuboids, as well as index.
		for(const auto& [fragment, value] : fragments)
			addToNodeRecursive(indexCopy, fragment, value);
	}
	Node& parent = m_nodes[indexCopy];
	addIntersectedChildrenToOpenList(parent, interceptMask, openList);
	m_toComb.maybeInsert(index);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::removeFromNodeWithValue(const RTreeNodeIndex& index, const Cuboid& cuboid, SmallSet<RTreeNodeIndex>& openList, const T& value)
{
	assert(value != T::create(nullPrimitive));
	CuboidArray<nodeSize>::BoolArray interceptMask;
	const RTreeNodeIndex indexCopy = index;
	// These node references are going to be invalidated by inserting fragments, which may generate a new node.
	// A scope is used to ensure they are not used after they are invalidated.
	{
		Node& parent = m_nodes[indexCopy];
		const auto& parentCuboids = parent.getCuboids();
		const auto& parentDataAndChildIndices = parent.getDataAndChildIndices();
		interceptMask = parentCuboids.indicesOfIntersectingCuboids(cuboid);
		// Fragment all intercepted leaves.
		SmallSet<Cuboid> fragments;
		const auto& leafEnd = parent.getLeafCount();
		for(RTreeArrayIndex i = RTreeArrayIndex::create(0); i < leafEnd; ++i)
			if(interceptMask[i.get()])
			{
				// Only remove leaves with the same value. Remove others from intercept mask so they aren't destroyed.
				if(parentDataAndChildIndices[i.get()].data == value.get())
					fragments.insertAll(parentCuboids[i.get()].getChildrenWhenSplitByCuboid(cuboid));
				else
					interceptMask[i.get()] = false;
			}
		// TODO: this could probably be better in regards to avoiding fragment generation rather then checking for empty. Will have to add more complex DEBUG logic.
		if constexpr(!config.splitAndMerge)
			assert(fragments.empty());
		// Copy intercept mask to create leaf intercept mask.
		auto leafInterceptMask = interceptMask;
		leafInterceptMask.tail(parent.getChildCount()) = 0;
		// erase all intercepted leaves with the same value.
		parent.eraseByMask(leafInterceptMask);
		// insert fragments. Invalidates parent and cuboids, as well as indexCopy.
		const auto end = fragments.end();
		for(auto i = fragments.begin(); i < end; ++i)
			addToNodeRecursive(indexCopy, *i, value);
	}
	Node& parent = m_nodes[indexCopy];
	addIntersectedChildrenToOpenList(parent, interceptMask, openList);
	m_toComb.maybeInsert(indexCopy);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::removeFromNodeByMask(Node& node, const Eigen::Array<bool, 1, Eigen::Dynamic>& mask)
{
		const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
		// Mark branches in m_node destroyed.
		for(RTreeArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			if(mask[i.get()])
				destroyWithChildren(RTreeNodeIndex::create(nodeDataAndChildIndices[i.get()].child));
		// Erase branches and leaves from node.
		node.eraseByMask(mask);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::updateBoundriesMaybe(const SmallSet<RTreeNodeIndex>& nodeIndices)
{
	for(const RTreeNodeIndex& index : nodeIndices)
	{
		assert(index != 0);
		Node& node = m_nodes[index];
		const RTreeNodeIndex parentIndex = node.getParent();
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
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::merge(const RTreeNodeIndex& destination, const RTreeNodeIndex& source)
{
	// TODO: maybe leave source nodes intact instead of striping out leaves?
	const auto leaves = gatherLeavesRecursive(source);
	for(const auto& [leaf, value] : leaves)
		addToNodeRecursive(destination, leaf, value);
}
// This method is identical to the one in rTreeBoolean, other then calling a different tryToMergeLeaves
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::comb()
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
			if constexpr(config.splitAndMerge)
				tryToMergeLeaves(node);
			// Try to merge child branches into this branch.
			bool merged = false;
			// plus one because we are removing one as well as adding.
			uint capacity = node.unusedCapacity() + 1;
			const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
			for(RTreeArrayIndex i = node.offsetOfFirstChild(); i < nodeSize; ++i)
			{
				// Take copy rather then reference because the original may be erased.
				const RTreeNodeIndex childIndex = RTreeNodeIndex::create(nodeDataAndChildIndices[i.get()].child);
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
				const RTreeNodeIndex& parentIndex = node.getParent();
				// Add one to capacity because merging will remove this branch.
				if((m_nodes[parentIndex].unusedCapacity() + 1) >= node.getChildCount() + node.getLeafCount())
					m_toComb.maybeInsert(parentIndex);
			}
		}
	}
	validate();
}
// This method is identical to the one in rTreeBoolean.
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::defragment()
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

		const auto& newNodeChildren = newNode.getDataAndChildIndices();
		for(auto i = newNode.offsetOfFirstChild(); i < nodeSize; ++i)
		{
			const RTreeNodeIndex& childIndex = RTreeNodeIndex::create(newNodeChildren[i.get()].child);
			assert(childIndex.exists());
			assert(m_nodes[childIndex].getParent() == last);
			m_nodes[childIndex].setParent(empty);
		}
	}
}
// This method is identical to the one in rTreeBoolean.
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::sort()
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
	for(RTreeNodeIndex oldIndex = RTreeNodeIndex::create(0); oldIndex < end; ++oldIndex)
	{
		const RTreeNodeIndex& newIndex = indices[oldIndex];
		Node& newNode = sortedNodes[newIndex];
		auto& dataAndChildren = newNode.getDataAndChildIndices();
		for(RTreeArrayIndex i = newNode.offsetOfFirstChild(); i != nodeSize; ++i)
		{
			const RTreeNodeIndex oldChildIndex = {dataAndChildren[i.get()].child};
			const RTreeNodeIndex newChildIndex = indices[oldChildIndex];
			dataAndChildren[i.get()].child = newChildIndex.get();
			sortedNodes[newChildIndex].setParent(newIndex);
		}
	}
	// validate.
	for(RTreeNodeIndex index = RTreeNodeIndex::create(0); index < sortedNodes.size(); ++index)
	{
		const Node& node = sortedNodes[index];
		if(index != 0)
		{
			const Node& parent = sortedNodes[node.getParent()];
			const RTreeArrayIndex offset = parent.offsetFor(index);;
			const auto& parentCuboids = parent.getCuboids();
			// Check that cuboid recorded in parent matches boundry of cuboids recorded in child.
			assert(parentCuboids[offset.get()] == node.getCuboids().boundry());
			const auto leafCount = node.getLeafCount();
			const RTreeNodeIndex oldIndex = indices.indexFor(index);
			const auto& oldDataAndChildren = m_nodes[oldIndex].getDataAndChildIndices();
			const auto& newDataAndChildren = node.getDataAndChildIndices();
			for(auto i = 0; i != leafCount; ++i)
				assert(newDataAndChildren[i].data == oldDataAndChildren[i].data);
			assert(node.getCuboids() == m_nodes[oldIndex].getCuboids());
		}
	}
	m_nodes = std::move(sortedNodes);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::addIntersectedChildrenToOpenList(const Node& node, const CuboidArray<nodeSize>::BoolArray& interceptMask, SmallSet<RTreeNodeIndex>& openList)
{
	const auto& nodeDataAndChildIndices = node.getDataAndChildIndices();
	const auto offsetOfFirstChild = node.offsetOfFirstChild();
	const uint8_t childCount = nodeSize - offsetOfFirstChild.get();
	if(childCount == 0 || !interceptMask.tail(childCount).any())
		return;
	auto begin = interceptMask.begin();
	auto end = begin + nodeSize;
	if(offsetOfFirstChild == 0)
	{
		// Unrollable version for nodes where every slot is a child.
		for(auto iter = begin; iter != end; ++iter)
			if(*iter)
				openList.insert(RTreeNodeIndex::create(nodeDataAndChildIndices[iter - begin].child));
	}
	else
	{
		for(auto iter = begin + offsetOfFirstChild.get(); iter != end; ++iter)
			if(*iter)
				openList.insert(RTreeNodeIndex::create(nodeDataAndChildIndices[iter - begin].child));
	}
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
RTreeData<T, config, nullPrimitive>::RTreeData()
{
	m_nodes.add();
	m_nodes.back().setParent(RTreeNodeIndex::null());
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::beforeJsonLoad() { m_nodes.clear(); }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeInsert(const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	if constexpr(!config.leavesCanOverlap)
		assert(!queryAny(cuboid));
	else
		for(const T& v : queryGetAll(cuboid))
			assert(canOverlap(v, value));
	constexpr RTreeNodeIndex zeroIndex = RTreeNodeIndex::create(0);
	[[maybe_unused]] bool breakIf = cuboid.volume() == 1 && cuboid.m_high == Point3D::create(20, 1, 1);
	addToNodeRecursive(zeroIndex, cuboid, value);
	validate();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeRemove(const Cuboid& cuboid)
{
	// Erase all contained branches and leaves.
	constexpr RTreeNodeIndex rootIndex = RTreeNodeIndex::create(0);
	clearAllContained(rootIndex, cuboid);
	if constexpr (!config.leavesCanOverlap && !config.splitAndMerge)
	{
		assert(!queryAny(cuboid));
		validate();
		return;
	}
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
	updateBoundriesMaybe(toUpdateBoundryMaybe);
	validate();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeRemove(const CuboidSet& cuboids)
{
	//TODO: optimize this like batch queries.
	for(const Cuboid& cuboid : cuboids)
		maybeRemove(cuboid);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeInsert(const CuboidSet& cuboids, const T& value)
{
	//TODO: optimize this like batch queries.
	for(const Cuboid& cuboid : cuboids)
		maybeInsert(cuboid, value);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeRemove(const Cuboid& cuboid, const T& value)
{
	assert(value != T::create(nullPrimitive));
	// Erase all contained branches and leaves.
	constexpr RTreeNodeIndex rootIndex = RTreeNodeIndex::create(0);
	clearAllContainedWithValueRecursive(m_nodes[rootIndex], cuboid, value);
	SmallSet<RTreeNodeIndex> openList;
	SmallSet<RTreeNodeIndex> toUpdateBoundryMaybe;
	openList.insert(rootIndex);
	while(!openList.empty())
	{
		auto index = openList.back();
		openList.popBack();
		removeFromNodeWithValue(index, cuboid, openList, value);
		//TODO: Check if cuboid is touching the boundry of the node at index from the inside. If not then no need to update the boundry.
		if(index != 0)
			toUpdateBoundryMaybe.maybeInsert(index);
	}
	updateBoundriesMaybe(toUpdateBoundryMaybe);
	validate();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeInsertOrOverwrite(const Cuboid& cuboid, const T& value)
{
	// TODO: turn this into a call to updateOrInsert?
	maybeRemove(cuboid);
	maybeInsert(cuboid, value);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::maybeInsertOrOverwrite(const Point3D& point, const T& value) { maybeInsertOrOverwrite({point, point}, value); }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::prepare()
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
	validate();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::clear()
{
	m_nodes.resize(1);
	m_nodes[RTreeNodeIndex::create(0)].clear();
	m_emptySlots.clear();
	m_toComb.clear();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
bool RTreeData<T, config, nullPrimitive>::anyLeafOverlapsAnother() const
{
	CuboidSet seen;
	for(const Cuboid& cuboid : getLeafCuboids())
	{
		if(seen.intersects(cuboid))
			return true;
		seen.maybeAdd(cuboid);
	}
	return false;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
Json RTreeData<T, config, nullPrimitive>::toJson() const
{
	Json output;
	output["nodes"] = Json::array();
	output["emptySlots"] = m_emptySlots;
	output["toComb"] = m_toComb;
	for(const Node& node : m_nodes)
		output["nodes"].push_back(node.toJson());
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
CuboidSet RTreeData<T, config, nullPrimitive>::getLeafCuboids() const
{
	CuboidSet output;
	for(const Node& node : m_nodes)
	{
		const auto leafCount = node.getLeafCount();
		const auto& cuboids = node.getCuboids();
		for(auto i = 0; i < leafCount; ++i)
			output.maybeAdd(cuboids[i]);
	}
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::validate() const
{
	if(!config.validate)
		return;
	assert(m_emptySlots.size() <= m_nodes.size());
	for(const RTreeNodeIndex& index : m_emptySlots)
		assert(index < m_nodes.size());
	SmallSet<RTreeNodeIndex> childIndices;
	for(RTreeNodeIndex index = RTreeNodeIndex::create(0); index < m_nodes.size(); ++index)
	{
		if(m_emptySlots.contains(index))
			return;
		const Node& node = m_nodes[index];
		const auto cuboids = node.getCuboids();
		// Check that cuboid recorded in parent matches boundry of cuboids recorded in child.
		if(index != 0)
		{
			if(m_emptySlots.contains(index))
				continue;
			const Node& parent = m_nodes[node.getParent()];
			const RTreeArrayIndex offset = parent.offsetFor(index);
			const auto& parentCuboids = parent.getCuboids();
			assert(parentCuboids[offset.get()] == cuboids.boundry());
		}
		// Check that leaves don't overlap unless allowed.
		const auto leafCount = node.getLeafCount();
		if constexpr(!config.leavesCanOverlap)
			for(uint i = 0; i < leafCount; ++i)
				assert(queryCount(cuboids[i]) == 1);
		const auto dataOrChildIndices = node.getDataAndChildIndices();
		// Check that leaf values are not null.
		for(uint i = 0; i < leafCount; ++i)
			assert(T::create(dataOrChildIndices[i].data) != T::create(nullPrimitive));
		// Check that child indices aren't in m_emptySlots and are unique.
		const auto nodeCount = m_nodes.size();
		for(RTreeArrayIndex i = node.offsetOfFirstChild(); i != nodeSize; ++i)
		{
			const RTreeNodeIndex childIndex = RTreeNodeIndex::create(dataOrChildIndices[i.get()].child);
			childIndices.insert(childIndex);
			assert(!m_emptySlots.contains(childIndex));
			assert(nodeCount > childIndex);
		}
	}
	// Check that indices in toComb are not also in m_emptySlots.
	for(const RTreeNodeIndex& index : m_toComb)
		assert(!m_emptySlots.contains(index));
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
void RTreeData<T, config, nullPrimitive>::load(const Json& data)
{
	for(const Json& nodeData : data["nodes"])
	{
		Node node;
		node.load(nodeData);
		m_nodes.add(node);
	}
	data["emptySlots"].get_to(m_emptySlots);
	data["toComb"].get_to(m_toComb);
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint32_t RTreeData<T, config, nullPrimitive>::nodeCount() const
{
	return m_nodes.size() - m_emptySlots.size();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint32_t RTreeData<T, config, nullPrimitive>::leafCount() const
{
	uint output = 0;
	for(RTreeNodeIndex i = RTreeNodeIndex::create(0); i < m_nodes.size(); ++i)
	{
		if(m_emptySlots.contains(i))
			continue;
		const Node& node = m_nodes[i];
		output += node.getLeafCount();
	}
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
const RTreeData<T, config, nullPrimitive>::Node& RTreeData<T, config, nullPrimitive>::getNode(uint i) const { return m_nodes[RTreeNodeIndex::create(i)]; }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
const Cuboid RTreeData<T, config, nullPrimitive>::getNodeCuboid(uint i, uint o) const { return m_nodes[RTreeNodeIndex::create(i)].getCuboids()[o]; }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
RTreeNodeIndex RTreeData<T, config, nullPrimitive>::getNodeChild(uint i, uint o) const { return RTreeNodeIndex::create(m_nodes[RTreeNodeIndex::create(i)].getDataAndChildIndices()[o].child); }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
T RTreeData<T, config, nullPrimitive>::queryPointOne(uint x, uint y, uint z) const { return queryGetOne(Point3D::create(x,y,z)); }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
T RTreeData<T, config, nullPrimitive>::queryPointFirst(uint x, uint y, uint z) const
{
	const auto found = queryGetAll(Point3D::create(x,y,z));
	if(found.empty())
		return T();
	return found.front();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
SmallSet<T> RTreeData<T, config, nullPrimitive>::queryPointAll(uint x, uint y, uint z) const { return queryGetAll(Point3D::create(x,y,z)); }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint8_t RTreeData<T, config, nullPrimitive>::queryPointCount(uint x, uint y, uint z) const { return queryCount(Point3D::create(x,y,z)); }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
Cuboid RTreeData<T, config, nullPrimitive>::queryPointCuboid(uint x, uint y, uint z) const
{
	static const Cuboid null;
	const CuboidSet& result = queryGetAllCuboids(Point3D::create(x,y,z));
	if(result.empty())
		return null;
	return result.front();
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint RTreeData<T, config, nullPrimitive>::totalNodeVolume() const
{
	uint output = 0;
	const auto end = m_nodes.size();
	for(RTreeNodeIndex index = RTreeNodeIndex::create(0); index < end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getNodeVolume();
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint RTreeData<T, config, nullPrimitive>::totalLeafVolume() const
{
	uint output = 0;
	const auto end = m_nodes.size();
	for(RTreeNodeIndex index = RTreeNodeIndex::create(0); index < end; ++index)
		if(!m_emptySlots.contains(index))
			output += m_nodes[index].getLeafVolume();
	return output;
}
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
uint RTreeData<T, config, nullPrimitive>::getNodeSize() { return nodeSize; }
template<Sortable T, RTreeDataConfig config, T::Primitive nullPrimitive>
std::string RTreeData<T, config, nullPrimitive>::toString(uint x, uint y, uint z) const
{
	if constexpr(HasToStringMethod<T>)
	{
		std::string output;
		for(const T& value : queryGetAll(Point3D::create(x, y, z)))
			output += value.toString();
		return output;
	}
	else
		assert(false);
}