/*
 * Memo data used for pathing, cleared and reused.
 * Pather is required to call reset before returning.
 */
#pragma once
#include "dataVector.h"
#include "index.h"
#include "onePassVector.h"
#include "types.h"
#include <cstdint>
#include <set>
#include "sortedVectorContainers.h"

class PathMemoClosed final
{
	DataVector<BlockIndex, BlockIndex> m_data;
	BlockIndices m_dirty;
public:
	[[nodiscard]] bool contains(BlockIndex index) const { return m_data[index].exists(); }
	[[nodiscard]] bool empty() const { return m_dirty.empty(); }
	[[nodiscard]] BlockIndex previous(BlockIndex index) const { assert(contains(index)); return m_data[index]; }
	[[nodiscard]] BlockIndices getPath(BlockIndex end) const;
	void add(BlockIndex index, BlockIndex parent);
	void clear() { for(BlockIndex index : m_dirty) { m_data[index].clear(); } m_dirty.clear(); }
	void resize(uint size) { m_data.resize(size); }
};

class PathMemoBreadthFirst final
{
	// TODO: Profile with single pass vector.
	std::deque<BlockIndex> m_open;
	PathMemoClosed m_closed;
public:
	PathMemoBreadthFirst(Area& area);
	void reset();
	void setClosed(BlockIndex block, BlockIndex previous);
	void setOpen(BlockIndex block);
	[[nodiscard]] bool isClosed(BlockIndex block) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] BlockIndex next();
	[[nodiscard]] BlockIndices getPath(BlockIndex end) const;
};
class PathMemoDepthFirst final
{
	MediumMap<DistanceInBlocks, BlockIndex> m_open;
	PathMemoClosed m_closed;
public:
	PathMemoDepthFirst(Area& area);
	void reset();
	void setClosed(BlockIndex block, BlockIndex previous);
	void setOpen(BlockIndex block, BlockIndex destinationHuristic, Area& area);
	[[nodiscard]] bool isClosed(BlockIndex block) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] BlockIndex next();
	[[nodiscard]] BlockIndices getPath(BlockIndex end) const;
};
class SimulationHasPathMemos final
{
	std::deque<PathMemoBreadthFirst> m_breadthFirst;
	std::deque<PathMemoDepthFirst> m_depthFirst;
	std::vector<bool> m_reservedBreadthFirst;
	std::vector<bool> m_reservedDepthFirst;
	std::mutex m_mutex;
public:
	std::pair<PathMemoBreadthFirst*, uint8_t> getBreadthFirst(Area& area);
	std::pair<PathMemoDepthFirst*, uint8_t> getDepthFirst(Area& area);
	void releaseBreadthFirst(uint8_t index);
	void releaseDepthFirst(uint8_t index);
};
