#pragma once
#include "rtreeData.h"
#include "../strongInteger.h"

template<typename T, typename DataIndexWidth, RTreeDataConfig config = RTreeDataConfig()>
class RTreeDataIndex
{
	class DataIndex : public StrongInteger<DataIndex, DataIndexWidth>
	{
	public:
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataIndex, data);
	};
	RTreeData<DataIndex, config> m_tree;
	StrongVector<std::pair<T, Cuboid>, DataIndex> m_data;
public:
	void insert(const Cuboid& cuboid, T&& value)
	{
		const DataIndex index = DataIndex::create(m_data.size());
		m_tree.maybeInsert(cuboid, index);
		m_data.emplaceBack(std::move(value), cuboid);
	}
	void remove(const Cuboid& cuboid)
	{
		const DataIndex index = m_tree.queryGetOne(cuboid);
		const DataIndex last = DataIndex::create(m_data.size() - 1);
		if(index != last)
		{
			m_data[index] = std::move(m_data[last]);
			m_tree.update(cuboid, last, index);
		}
		m_data.popBack();
		m_tree.maybeRemove(cuboid);
	}
	void insert(const Point3D& point, T&& value) { insert({point, point}, std::move(value)); }
	void remove(const Point3D& point) { remove({point, point}); }
	void updateOne(const auto& shape, const T& oldValue, T&& newValue)
	{
		const DataIndex& index = m_tree.queryGetOne(shape);
		assert(m_data[index].first == oldValue);
		m_data[index].first = std::move(newValue);
	}
	void updateOne(const auto& shape, auto&& action)
	{
		const DataIndex& index = m_tree.queryGetOne(shape);
		action(m_data[index].first);
	}
	[[nodiscard]] bool queryAny(const auto& shape) const { return m_tree.queryAny(shape); }
	[[nodiscard]] const T& queryGetOne(const auto& shape) const
	{
		const DataIndex& index = m_tree.queryGetOne(shape);
		return m_data[index].first;
	}
	[[nodiscard]] T& queryGetOneMutable(const auto& shape)
	{
		assert(!config.splitAndMerge);
		const DataIndex& index = m_tree.queryGetOne(shape);
		return m_data[index].first;
	}
	[[nodiscard]] SmallSet<std::pair<Cuboid, const T*>> queryGetAllWithCuboids(const auto& shape) const
	{
		SmallSet<std::pair<Cuboid, const T*>> output;
		const SmallSet<std::pair<Cuboid, DataIndex>>& indexesWithCuboids = m_tree.queryGetAllWithCuboids(shape);
		for(auto& [cuboid, index] : indexesWithCuboids)
			output.emplace(cuboid, &m_data[index].first);
		return output;
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RTreeDataIndex, m_tree, m_data);
};
