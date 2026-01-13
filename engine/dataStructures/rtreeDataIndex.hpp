#pragma once
#include "rtreeDataIndex.h"
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::insert(const Cuboid& cuboid, T&& value)
{
	const DataIndex index = DataIndex::create(m_data.size());
	m_tree.maybeInsert(cuboid, index);
	m_data.emplaceBack(std::move(value), cuboid);
}
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::insertOrOverwrite(const Cuboid& cuboid, T&& value)
{
	const DataIndex found = m_tree.queryGetOne(cuboid);
	if(found.exists())
	{
		m_data[found].first = std::move(value);
		assert(m_data[found].second == cuboid);
	}
	else
	{
		const DataIndex index = DataIndex::create(m_data.size());
		m_tree.maybeInsert(cuboid, index);
		m_data.emplaceBack(std::move(value), cuboid);
	}
}
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::insert(const Point3D& point, T&& value) { insert({point, point}, std::move(value)); }
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::insertOrOverwrite(const Point3D& point, T&& value) { insertOrOverwrite({point, point}, std::move(value)); }
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::remove(const Point3D& point) { remove(Cuboid{point, point}); }
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::clear() { m_tree.clear(); m_data.clear(); }
template<typename T, RTreeDataConfig config>
void RTreeDataIndex<T, config>::prepare() { m_tree.prepare(); }
template<typename T, RTreeDataConfig config>
bool  RTreeDataIndex<T, config>::canPrepare() const { return m_tree.canPrepare(); }
template<typename T, RTreeDataConfig config>
bool RTreeDataIndex<T, config>::empty() const { return m_tree.empty(); }
template<typename T, RTreeDataConfig config>
CuboidSet RTreeDataIndex<T, config>::getLeafCuboids() const
{
	CuboidSet output;
	for(const auto& pair : m_data)
		output.maybeAdd(pair.second);
	return output;
}