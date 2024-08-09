#include "pathMemo.h"
#include "area.h"
#include "blocks/blocks.h"
#include "index.h"
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
	assert(!isClosed(block));
	m_closed.add(block, previous);
}
void PathMemoBreadthFirst::setOpen(BlockIndex block)
{
	assert(!isClosed(block));
	m_open.add(block);
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
	return m_open.pop();
}
BlockIndices PathMemoBreadthFirst::getPath(BlockIndex block) const
{
	BlockIndices output;
	while(m_closed.contains(block))
	{
		output.add(block);
		block = m_closed.previous(block);
	}
	return output;
}
// Depth First.
PathMemoDepthFirst::PathMemoDepthFirst(Area& area)
{
	m_closed.resize(area.getBlocks().size());
}
void PathMemoDepthFirst::reset()
{
	m_closed.clear();
	m_vector.clear();
}
void PathMemoDepthFirst::setup(Area& area, BlockIndex block)
{
	Blocks& blocks = area.getBlocks();
	Comparitor condition = [&](const BlockIndex& a, const BlockIndex& b){
		return blocks.taxiDistance(block, a) < blocks.taxiDistance(block, b);
	};
	m_open = std::priority_queue<BlockIndex, std::vector<BlockIndex>, decltype(condition)>(condition, m_vector);
}
void PathMemoDepthFirst::setClosed(BlockIndex block, BlockIndex previous)
{
	assert(!isClosed(block));
	m_closed.add(block, previous);
}
void PathMemoDepthFirst::setOpen(BlockIndex block)
{
	assert(!isClosed(block));
	m_open.push(block);
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
	BlockIndex output = m_open.top();
	m_open.pop();
	return output;
}
BlockIndices PathMemoDepthFirst::getPath(BlockIndex block) const
{
	BlockIndices output;
	while(m_closed.contains(block))
	{
		output.add(block);
		block = m_closed.previous(block);
	}
	return output;
}
// SimulaitonHasPathMemos.
std::vector<PathMemoBreadthFirst>& SimulationHasPathMemos::getBreadthFirstWithMinimumSize(uint size, Area& area)
{
	if(m_breadthFirst.size() < size)
	{
		m_breadthFirst.resize(size, area);
		m_reservedBreadthFirst.resize(size);
	}
	return m_breadthFirst;
}
std::vector<PathMemoDepthFirst>& SimulationHasPathMemos::getDepthFirstWithMinimumSize(uint size, Area& area)
{
	if(m_depthFirst.size() < size)
	{
		m_depthFirst.resize(size, area);
		m_reservedDepthFirst.resize(size);
	}
	return m_depthFirst;
}
std::pair<PathMemoBreadthFirst&, uint8_t> SimulationHasPathMemos::getBreadthFirstSingle(Area& area)
{
	std::lock_guard lock(m_mutex);
	// Find an unreserved memo to use.
	auto found = std::ranges::find(m_reservedBreadthFirst, false);
	if(found == m_reservedBreadthFirst.end())
	{
		uint oldSize = m_breadthFirst.size();
		m_breadthFirst.resize(m_breadthFirst.size() * 1.5, area);
		m_reservedBreadthFirst.resize(m_breadthFirst.size());
		found = m_reservedBreadthFirst.begin() + oldSize;
	}
	uint offset = found - m_reservedBreadthFirst.begin();
	(*found) = true;
	return {m_breadthFirst.at(offset), offset};
}
std::pair<PathMemoDepthFirst&, uint8_t> SimulationHasPathMemos::getDepthFirstSingle(Area& area)
{
	std::lock_guard lock(m_mutex);
	// Find an unreserved memo to use.
	auto found = std::ranges::find(m_reservedDepthFirst, false);
	if(found == m_reservedDepthFirst.end())
	{
		uint oldSize = m_depthFirst.size();
		m_depthFirst.resize(m_depthFirst.size() * 1.5, area);
		m_reservedDepthFirst.resize(m_depthFirst.size());
		found = m_reservedDepthFirst.begin() + oldSize;
	}
	uint offset = found - m_reservedDepthFirst.begin();
	(*found) = true;
	return {m_depthFirst.at(offset), offset};
}
void SimulationHasPathMemos::releaseBreadthFirstSingle(uint8_t index)
{
	std::lock_guard lock(m_mutex);
	m_reservedBreadthFirst.at(index) = false;
	assert(m_breadthFirst.at(index).empty());
}
void SimulationHasPathMemos::releaseDepthFirstSingle(uint8_t index)
{
	std::lock_guard lock(m_mutex);
	m_reservedBreadthFirst.at(index) = false;
	assert(m_depthFirst.at(index).empty());
}
