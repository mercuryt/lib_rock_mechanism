#pragma once
#include "rtreeData.hpp"
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
	T m_empty;
	void removeInternal(const auto& shape, const DataIndex& index)
	{
		assert(index.exists());
		assert(m_data.size() > index);
		const DataIndex last = DataIndex::create(m_data.size() - 1);
		if(index != last)
		{
			m_data[index] = std::move(m_data[last]);
			m_tree.update(shape, last, index);
		}
		m_data.popBack();
		m_tree.maybeRemove(shape);
	}
public:
	void insert(const Cuboid& cuboid, T&& value);
	void insertOrOverwrite(const Cuboid& cuboid, T&& value);
	void insert(const Point3D& point, T&& value);
	void insertOrOverwrite(const Point3D& point, T&& value);
	void remove(const auto& shape)
	{
		const DataIndex index = m_tree.queryGetOne(shape);
		removeInternal(shape, index);
	}
	void remove(const Point3D& point);
	void maybeRemove(const auto& shape)
	{
		const DataIndex index = m_tree.queryGetOne(shape);
		if(index.exists())
			removeInternal(shape, index);
	}
	void updateOne(const auto& shape, const T& oldValue, T&& newValue)
	{
		const DataIndex& index = m_tree.queryGetOne(shape);
		assert(index.exists());
		assert(m_data[index].first == oldValue);
		m_data[index].first = std::move(newValue);
	}
	void updateOne(const auto& shape, auto&& action)
	{
		const auto& [index, cuboid] = m_tree.queryGetOneWithCuboid(shape);
		assert(index.exists());
		assert(cuboid.contains(shape) && shape.contains(cuboid));
		action(m_data[index].first);
	}
	void updateOrInsertOne(const auto& shape, auto&& action)
	{
		const auto& [index, cuboid] = m_tree.queryGetOneWithCuboid(shape);
		if(index.exists())
		{
			assert(cuboid.contains(shape) && shape.contains(cuboid));
			action(m_data[index].first);
		}
		else
		{
			T value;
			action(value);
			insert(shape, std::move(value));
		}
	}
	void clear();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool queryAny(const auto& shape) const { return m_tree.queryAny(shape); }
	[[nodiscard]] const T& queryGetOne(const auto& shape) const
	{
		const DataIndex& index = m_tree.queryGetOne(shape);
		if(index.empty())
			return m_empty;
		return m_data[index].first;
	}
	[[nodiscard]] const T& queryGetOneOr(const auto& shape, const T& alternate) const
	{
		const DataIndex& index = m_tree.queryGetOne(shape);
		if(index.empty())
			return alternate;
		return m_data[index].first;
	}
	[[nodiscard]] T& queryGetOneMutable(const auto& shape)
	{
		assert(!config.splitAndMerge);
		const DataIndex& index = m_tree.queryGetOne(shape);
		assert(index.exists());
		return m_data[index].first;
	}

	struct ConstIterator
	{
		SmallSet<DataIndex>::const_iterator m_iter;
		const StrongVector<std::pair<T, Cuboid>, DataIndex>& m_data;
		ConstIterator(const SmallSet<DataIndex>& indices, const StrongVector<std::pair<T, Cuboid>, DataIndex>& data, bool end = false) :
			m_iter(end ? indices.end() : indices.begin()),
			m_data(data)
		{ }
		ConstIterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] ConstIterator operator++(int) { auto copy = *this; ++m_iter; return copy;}
		[[nodiscard]] const T& operator*() const { return m_data[*m_iter].first; }
		[[nodiscard]] bool operator==(const ConstIterator& other) const { return m_iter == other.m_iter; }
	};
	struct QueryViewConst
	{
		const SmallSet<DataIndex> m_indicies;
		const StrongVector<std::pair<T, Cuboid>, DataIndex>& m_data;
		ConstIterator begin() const { return {m_indicies, m_data}; }
		ConstIterator end() const { return {m_indicies, m_data, true}; }
	};
	[[nodiscard]] QueryViewConst queryGetAll(const auto& shape) const
	{
		return QueryViewConst(m_tree.queryGetAll(shape), m_data);
	}
	struct ConstIteratorWithCuboids
	{
		SmallSet<DataIndex>::const_iterator m_iter;
		const StrongVector<std::pair<T, Cuboid>, DataIndex>& m_data;
		ConstIteratorWithCuboids(const SmallSet<DataIndex>& indices, const StrongVector<std::pair<T, Cuboid>, DataIndex>& data, bool end = false) :
			m_iter(end ? indices.end() : indices.begin()),
			m_data(data)
		{ }
		ConstIteratorWithCuboids& operator++() { ++m_iter; return *this; }
		[[nodiscard]] ConstIteratorWithCuboids operator++(int) { auto copy = *this; ++m_iter; return copy;}
		[[nodiscard]] const std::pair<T, Cuboid>& operator*() const { return m_data[*m_iter]; }
		[[nodiscard]] bool operator==(const ConstIteratorWithCuboids& other) const { return m_iter == other.m_iter; }
	};
	struct QueryViewConstWithCuboids
	{
		const SmallSet<DataIndex> m_indicies;
		const StrongVector<std::pair<T, Cuboid>, DataIndex>& m_data;
		ConstIteratorWithCuboids begin() const { return {m_indicies, m_data}; }
		ConstIteratorWithCuboids end() const { return {m_indicies, m_data, true}; }
	};
	[[nodiscard]] QueryViewConstWithCuboids queryGetAllWithCuboids(const auto& shape) const
	{
		return QueryViewConstWithCuboids(m_tree.queryGetAll(shape), m_data);
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RTreeDataIndex, m_tree, m_data);
};
