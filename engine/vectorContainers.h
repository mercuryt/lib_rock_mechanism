#pragma once
/*
 * To be used for short seqences of data, not long enough for binary search to make sense.
 * Not pointer stable.
 */
#include "util.h"
#include "json.h"
#include <vector>
#include <ranges>
#include <cassert>

template<typename T>
class SmallSet
{
	std::vector<T> m_data;
public:
	SmallSet() = default;
	void load(const Json& data) { m_data = data; }
	void insert(const T& value) { assert(!contains(value)); m_data.push_back(value); }
	void erase(const T& value) { util::removeFromVectorByValueUnordered(m_data, value); }
	void clear() { m_data.clear(); }
	[[nodiscard]] bool contains(const T& value) const { return std::ranges::find(m_data, value) != m_data.end(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] std::vector<T>::iterator begin() { return m_data.begin(); }
	[[nodiscard]] std::vector<T>::iterator end() { return m_data.end(); }
	[[nodiscard]] std::vector<T>::const_iterator begin() const { return m_data.begin(); }
	[[nodiscard]] std::vector<T>::const_iterator end() const { return m_data.end(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallSet, m_data);
};
template<typename T>
inline void from_json(const Json& data, SmallSet<T>& set) { set.load(data); }
// Used in place of boost::unordered_flat_map because we need to have an getAllKeys method.
template<typename K, typename V>
class SmallMap
{
	std::vector<K> keys;
	std::vector<V> values;
public:
	void insert(K& key, V& value) 
	{ 
		assert(!contains(key));
		keys.push_back(key);
		values.push_back(value);
	}
	void erase(K& key)
	{
		if(size() == 1)
			clear();
		auto iter = find(key);
		auto iter2 = values.begin() + (std::distance(iter, keys.begin()));
		(*iter) = keys.back();
		(*iter2) = values.back();
		keys.pop_back();
		values.pop_back();
	}
	void clear() 
	{ 
		keys.clear(); 
		values.clear();
	}
	[[nodiscard]] uint size() const { return keys.size(); }
	[[nodiscard]] V& operator[](K& key) 
	{
	       	assert(contains(key)); 
		auto iterKey = find(key);
		auto iter2 = at(iterKey);
		return *iter2; 
	}
	[[nodiscard]] bool contains(K& key) { return findKey(key) != keys.end(); }
	[[nodiscard]] std::vector<K>::iterator findKey(K& key) { return std::ranges::find(keys, key); }
	[[nodiscard]] V& at(std::vector<K>::iterator iterKey) { return values.begin() + (std::distance(iterKey, keys.begin())); }
	class Iterator
	{
		std::vector<K>::iterator m_iterator;
		SmallMap<K, V>& m_map;
	public:
		Iterator(SmallMap<K, V>& map, std::vector<K>::iterator iterator) : m_iterator(iterator), m_map(map) { }
		void operator++() { m_iterator++; }
		[[nodiscard]] std::pair<K&, V&> operator*() { return {m_iterator, m_map.at(m_iterator)}; }
		[[nodiscard]] bool operator==(Iterator& iterator) { return m_iterator == iterator.m_iterator; }
	};
	[[nodiscard]] Iterator begin() { return {*this, keys.begin()}; }
	[[nodiscard]] Iterator end() { return {*this, keys.end()}; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallMap, keys, values);
};
template<typename K, typename V>
class SmallMapSortedDescending : public SmallMap<K, V>
{
	void insert(K& key, V& value) { 
		assert(!contains(key)); 
		auto& data = SmallMap<K, V>::data;
		data.emplace_back(key, value); 
		std::ranges::sort(data, std::ranges::greater{}, std::pair<K, V>::first);
	}
};
