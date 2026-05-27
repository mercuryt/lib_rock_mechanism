#include "closedListLongRange.h"
void ClosedListLongRange::initalize(const Cuboid startCuboid, SmallSet<Cuboid>&& connected)
{
	// Start cuboid has no from.
	m_data.emplace_back(startCuboid, Cuboid::null(), std::move(connected));
}
void ClosedListLongRange::close(const Cuboid to, const Cuboid from, SmallSet<Cuboid>&& connected)
{
	assert(to.exists());
	assert(from.exists());
	// Check for redundant nodes.
	if(connected.empty())
		// This node was created with long range mode, no need to check from.
		assert(!std::ranges::contains(m_data, to, &HistoryNode::to));
	else
		// This node was created with short range mode, much use from to differentiate between other nodes with the same to.
		assert(std::ranges::find_if(m_data, [to , from](const HistoryNode& node) { return node.to == to && node.from == from; }) == m_data.end());
	m_data.emplace_back(to, from, std::move(connected));
}
bool ClosedListLongRange::checkWithFrom(const Cuboid to, const Cuboid from) const
{
	auto found = std::ranges::find_if(m_data, [to, from](const HistoryNode& node){ return node.to == to && node.connected.contains(from); });
	return found != m_data.end();
}
bool ClosedListLongRange::checkWithoutFrom(const Cuboid to) const
{
	return std::ranges::contains(m_data, to, &HistoryNode::to);
}
std::vector<Cuboid> ClosedListLongRange::getPath(const Cuboid end, Cuboid previous) const
{
	std::vector<Cuboid> output;
	output.push_back(end);
	// Where we currently are in the path.
	Cuboid current = previous;
	// The next cuboid in the path, which is the previous one processed since we are going backwards.
	Cuboid next = end;
	while(current.exists())
	{
		output.push_back(current);
		Cuboid newCurrent = getPrevious(current, next);
		next = current;
		current = newCurrent;
	}
	return output;
}
Cuboid ClosedListLongRange::getPrevious(const Cuboid to, const Cuboid connected) const
{
	const auto found = std::ranges::find_if(m_data, [to, connected](const HistoryNode& node){ return node.to == to && (node.connected.empty() || node.connected.contains(connected)); });
	assert(found != m_data.end());
	return found->from;
}

