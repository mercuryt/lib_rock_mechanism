#include "pathMemo.h"
#include "area.h"
#include "blocks/blocks.h"
#include "index.h"
void PathMemoClosed::add(BlockIndex index, BlockIndex parent)
{
	assert(!contains(index));
	assert(index.exists());
	// Max is used to indicate the start of the path.
	if(parent != BlockIndex::max())
		assert(contains(parent));
	m_data[index] = parent;
	m_dirty.add(index);
}
BlockIndices PathMemoClosed::getPath(BlockIndex block) const
{
	BlockIndices output;
	// Max is used to indicade the start of the path.
	while(previous(block) != BlockIndex::max())
	{
		output.add(block);
		block = previous(block);
	}
	std::ranges::reverse(output);
	return output;
}
// Breadth First.
PathMemoBreadthFirst::PathMemoBreadthFirst(Area& area)
{
	m_closed.resize(area.getBlocks().size());
}
void PathMemoBreadthFirst::reset()
{
	m_closed.clear();
	m_open.clear();
}
void PathMemoBreadthFirst::setClosed(BlockIndex block, BlockIndex previous)
{
	assert(!m_closed.contains(block));
	m_closed.add(block, previous);
}
void PathMemoBreadthFirst::setOpen(BlockIndex block)
{
	assert(!m_closed.contains(block));
	m_open.push_back(block);
}
bool PathMemoBreadthFirst::isClosed(BlockIndex block) const
{
	return m_closed.contains(block);
}
bool PathMemoBreadthFirst::empty() const
{
	return m_closed.empty() && m_open.empty();
}
BlockIndex PathMemoBreadthFirst::next()
{
	auto output = m_open.front();
	m_open.pop_front();
	return output;
}
BlockIndices PathMemoBreadthFirst::getPath(BlockIndex block) const
{
	return m_closed.getPath(block);
}
// Depth First.
PathMemoDepthFirst::PathMemoDepthFirst(Area& area)
{
	m_closed.resize(area.getBlocks().size());
}
void PathMemoDepthFirst::reset()
{
	m_closed.clear();
	m_open.clear();
}
void PathMemoDepthFirst::setClosed(BlockIndex block, BlockIndex previous)
{
	assert(!m_closed.contains(block));
	m_closed.add(block, previous);
}
void PathMemoDepthFirst::setOpen(BlockIndex block, BlockIndex destinationHuristic, Area& area)
{
	bool contains = m_closed.contains(block);
	assert(!contains);
	// Subtract from max rather then provide MediumMap with the ability to sort backwards.
	DistanceInBlocks distance = DistanceInBlocks::max() - area.getBlocks().distanceSquared(block, destinationHuristic);
	m_open.insertNonUnique(distance, block);
}
bool PathMemoDepthFirst::isClosed(BlockIndex block) const
{
	return m_closed.contains(block);
}
bool PathMemoDepthFirst::empty() const
{
	return m_closed.empty() && m_open.empty();
}
BlockIndex PathMemoDepthFirst::next()
{
	BlockIndex output = m_open.back().second;
	m_open.pop_back();
	return output;
}
BlockIndices PathMemoDepthFirst::getPath(BlockIndex block) const
{
	return m_closed.getPath(block);
}
// SimulaitonHasPathMemos.
std::pair<PathMemoBreadthFirst*, uint8_t> SimulationHasPathMemos::getBreadthFirst(Area& area)
{
	std::lock_guard lock(m_mutex);
	// Find an unreserved memo to use.
	auto found = std::ranges::find(m_reservedBreadthFirst, false);
	if(found == m_reservedBreadthFirst.end())
	{
		uint oldSize = m_breadthFirst.size();
		m_breadthFirst.emplace_back(area);
		m_reservedBreadthFirst.resize(m_breadthFirst.size());
		found = m_reservedBreadthFirst.begin() + oldSize;
	}
	uint offset = std::distance(m_reservedBreadthFirst.begin(), found);
	m_reservedBreadthFirst[offset] = true;
	return {&m_breadthFirst[offset], offset};
}
std::pair<PathMemoDepthFirst*, uint8_t> SimulationHasPathMemos::getDepthFirst(Area& area)
{
	std::lock_guard lock(m_mutex);
	// Find an unreserved memo to use.
	auto found = std::ranges::find(m_reservedDepthFirst, false);
	if(found == m_reservedDepthFirst.end())
	{
		uint oldSize = m_depthFirst.size();
		m_depthFirst.emplace_back(area);
		m_reservedDepthFirst.resize(m_depthFirst.size());
		found = m_reservedDepthFirst.begin() + oldSize;
	}
	uint offset = std::distance(m_reservedDepthFirst.begin(), found);
	m_reservedDepthFirst[offset] = true;
	return {&m_depthFirst[offset], offset};
}
void SimulationHasPathMemos::releaseBreadthFirst(uint8_t index)
{
	std::lock_guard lock(m_mutex);
	m_reservedBreadthFirst[index] = false;
	assert(m_breadthFirst[index].empty());
}
void SimulationHasPathMemos::releaseDepthFirst(uint8_t index)
{
	std::lock_guard lock(m_mutex);
	m_reservedDepthFirst[index] = false;
	assert(m_depthFirst[index].empty());
}
