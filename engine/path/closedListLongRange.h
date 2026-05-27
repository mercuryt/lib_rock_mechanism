#pragma once
#include "../geometry/cuboid.h"
#include "../dataStructures/smallMap.h"
struct HistoryNode
{
	const Cuboid to;
	const Cuboid from;
	SmallSet<Cuboid> connected;
};
struct ClosedListLongRange
{
	std::vector<HistoryNode> m_data;
	void initalize(const Cuboid cuboid, SmallSet<Cuboid>&& connected);
	void close(const Cuboid to, const Cuboid from, SmallSet<Cuboid>&& connected);
	void clear() { m_data.clear(); }
	[[nodiscard]] bool checkWithFrom(const Cuboid to, const Cuboid from) const;
	[[nodiscard]] bool checkWithoutFrom(const Cuboid to) const;
	[[nodiscard]] std::vector<Cuboid> getPath(const Cuboid end, Cuboid previous) const;
	Cuboid getPrevious(const Cuboid to, const Cuboid from) const;
};