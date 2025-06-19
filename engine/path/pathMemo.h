/*
 * Memo data used for pathing, cleared and reused.
 * Pather is required to call reset before returning.
 */
#pragma once
#include "dataStructures/strongVector.h"
#include "dataStructures/sortedVectorContainers.h"
#include "index.h"
#include "types.h"
#include <cstdint>
#include <set>

class PathMemoClosed final
{
	StrongVector<BlockIndex, BlockIndex> m_data;
	SmallSet<BlockIndex> m_dirty;
public:
	[[nodiscard]] bool contains(const BlockIndex& index) const { return m_data[index].exists(); }
	[[nodiscard]] bool empty() const { return m_dirty.empty(); }
	[[nodiscard]] BlockIndex previous(const BlockIndex& index) const { assert(contains(index)); return m_data[index]; }
	[[nodiscard]] SmallSet<BlockIndex> getPath(const BlockIndex& secondToLast, const BlockIndex& last) const;
	[[nodiscard]] uint getSize() const { return m_data.size(); }
	void add(const BlockIndex& index, BlockIndex parent);
	void clear() { for(const BlockIndex& index : m_dirty) { m_data[index].clear(); } m_dirty.clear(); }
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
	void setClosed(const BlockIndex& block, const BlockIndex& previous);
	// Area is not used here, it is only provided to match the interface of depth first.
	void setOpen(const BlockIndex& block, const Area& area);
	[[nodiscard]] bool isClosed(const BlockIndex& block) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] BlockIndex next();
	[[nodiscard]] SmallSet<BlockIndex> getPath(const BlockIndex& secondToLast, const BlockIndex& last) const;
};
class PathMemoDepthFirst final
{
	MediumMap<DistanceInBlocks, BlockIndex> m_open;
	PathMemoClosed m_closed;
	BlockIndex m_huristicDestination;
public:
	PathMemoDepthFirst(Area& area);
	void reset();
	void setClosed(const BlockIndex& block, const BlockIndex& previous);
	void setOpen(const BlockIndex& block, const Area& area);
	void setDestination(const BlockIndex& block) { m_huristicDestination = block; }
	[[nodiscard]] bool isClosed(const BlockIndex& block) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool openEmpty() const { return m_open.empty(); }
	[[nodiscard]] BlockIndex next();
	[[nodiscard]] SmallSet<BlockIndex> getPath(const BlockIndex& secondToLast, const BlockIndex& last) const;
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
