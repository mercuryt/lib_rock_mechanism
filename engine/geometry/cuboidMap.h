#pragma once

#include "cuboid.h"
#include "cuboidSetSIMD.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include <vector>

template<typename Key>
class CuboidMap
{
	CuboidSetSIMD m_cuboids;
	std::vector<Key> m_keys;
public:
	CuboidMap() : CuboidMap(8) { }
	CuboidMap(uint capacity) : m_cuboids(capacity) { m_keys.reserve(capacity); }
	CuboidMap(const CuboidMap&) = default;
	CuboidMap(CuboidMap&&) = default;
	void operator=(const CuboidMap& other) { m_cuboids = other.m_cuboids; m_keys = other.m_keys; }
	void operator=(CuboidMap&& other) { m_cuboids = std::move(other.m_cuboids); m_keys = std::move(other.m_keys); }
	void insert(const Key& key, const Cuboid& cuboid)
	{
		assert(key.exists());
		assert(cuboid.exists());
		assert(!contains(key));
		m_keys.push_back(key);
		m_cuboids.insert(cuboid);
	}
	void erase(const Key& key)
	{
		auto found = std::ranges::find(m_keys, key);
		assert(found != m_keys.end());
		uint index = std::distance(m_keys.begin(), found);
		if(index != m_keys.size() - 1)
			m_keys[index] = m_keys[m_keys.size() - 1];
		m_keys.pop_back();
		m_cuboids.erase(index);
	}
	void maybeErase(const Key& key)
	{
		auto found = std::ranges::find(m_keys, key);
		if(found != m_keys.end())
		{
			uint index = std::distance(m_keys.begin(), found);
			if(index != m_keys.size() - 1)
				m_keys[index] = m_keys[m_keys.size() - 1];
			m_keys.pop_back();
			m_cuboids.erase(index);
		}
	}
	void update(const Key& oldKey, const Key& newKey, const Cuboid& cuboid)
	{
		assert(newKey.exists());
		assert(cuboid.exists());
		auto found = std::ranges::find(m_keys, oldKey);
		assert(found != m_keys.end());
		uint index = std::distance(m_keys.begin(), found);
		assert(m_cuboids[index] == cuboid);
		(*found) = newKey;
	}
	void update(const Key& key, const Cuboid& cuboid)
	{
		assert(key.exists());
		assert(cuboid.exists());
		auto found = std::ranges::find(m_keys, key);
		assert(found != m_keys.end());
		uint index = std::distance(m_keys.begin(), found);
		m_cuboids.update(index, cuboid);
	}
	void clear()
	{
		m_cuboids.clear();
		m_keys.clear();
	}
	[[nodiscard]] CuboidSetSIMD& cuboids() { return m_cuboids; }
	[[nodiscard]] bool contains(const Key& key) const { return std::ranges::contains(m_keys, key); }
	[[nodiscard]] bool containsAsMember(const Cuboid& cuboid) const { return m_cuboids.containsAsMember(cuboid); }
	[[nodiscard]] bool empty() const { return m_keys.empty(); }
	[[nodiscard]] uint size() const { return m_keys.size(); }
	[[nodiscard]] const auto& keys() const { return  m_keys; }
	[[nodiscard]] Cuboid operator[](const Key& key) const { auto found = std::ranges::find(m_keys, key); assert(found != m_keys.end()); uint index = std::distance(m_keys.begin(), found); return m_cuboids[index];}
	[[nodiscard]] Cuboid byIndex(const uint& index) const { return m_cuboids[index]; }
	[[nodiscard]] std::string toString() const
	{
		std::string output;
		for(uint i = 0; i < m_keys.size(); ++i)
			output += m_keys[i].toString() + ": " + m_cuboids[i].toString() + ", ";
		return output;
	}
	class ConstIterator
	{
		const CuboidMap& m_map;
		uint m_index = 0;
	public:
		ConstIterator(const CuboidMap& map, uint index) : m_map(map), m_index(index) { }
		void operator++() { ++m_index; }
		[[nodiscard]] ConstIterator operator++(int) { auto copy = *this; ++(*this); return copy; }
		[[nodiscard]] std::pair<Key, Cuboid> operator*() const { return { m_map.m_keys[m_index], m_map.m_cuboids[m_index] }; }
		[[nodiscard]] bool operator==(const ConstIterator& other) const { assert(&m_map == &other.m_map); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) const { return !((*this) == other); }
	};
	ConstIterator begin() const { return ConstIterator{*this, 0}; }
	ConstIterator end() const { return ConstIterator{*this, (uint)m_keys.size()}; }
	class Query
	{
		const CuboidMap& m_map;
		const Eigen::Array<bool, 1, Eigen::Dynamic> m_mask;
	public:
		Query(const CuboidMap& map, const auto& queryShape) :
			m_map(map),
			m_mask(m_map.m_cuboids.indicesOfIntersectingCuboids(queryShape))
		{ }
		class Iterator
		{
			const Query& m_query;
			uint m_index = 0;
		public:
			Iterator(const Query& query, uint index) : m_query(query), m_index(index)
			{
				while(m_index < m_query.m_mask.size() && !m_query.m_mask[m_index])
					++m_index;
			}
			void operator++()
			{
				do
					++m_index;
				while(m_index < m_query.m_mask.size() && !m_query.m_mask[m_index]);
			}
			[[nodiscard]] Iterator operator++(int) { auto copy = *this; ++(*this); return copy; }
			[[nodiscard]] Key operator*() const { return m_query.m_map.m_keys[m_index]; }
			[[nodiscard]] bool operator==(const Iterator& other) const { assert(&m_query == &other.m_query); return m_index == other.m_index; }
			[[nodiscard]] bool operator!=(const Iterator& other) const { return !((*this) == other); }
		};
		Iterator begin() const { return Iterator{*this, 0}; }
		Iterator end() const { return Iterator{*this, (uint)m_mask.size()}; }
		friend class Iterator;
	};
	Query query(const auto& queryShape) const { return Query(*this, queryShape); }
	friend class Query;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(CuboidMap, m_cuboids, m_keys);
};