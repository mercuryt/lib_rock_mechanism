#pragma once
/*
 * To be used for short seqences of data, not long enough for binary search to make sense.
 * Not pointer stable.
 */
#include "util.h"
#include <vector>
#include <ranges>
#include <cassert>

template<typename T>
class SmallSet
{
	std::vector<T> data;
public:
	void insert(T& value) { assert(!contains(value)); data.push_back(value); }
	void erase(T& value) { util::removeFromVectorByValueUnordered(data, value); }
	void clear() { data.clear(); }
	[[nodiscard]] bool contains(T& value) { return std::ranges::find(data, value) != data.end(); }
	[[nodiscard]] std::vector<T> begin() { return data.begin(); }
	[[nodiscard]] std::vector<T> end() { return data.end(); }
};
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
	[[nodiscard]] V& at(K& key) 
	{
	       	assert(contains(key)); 
		auto iter = find(key);
		auto iter2 = values.begin() + (std::distance(iter, keys.begin()));
		return *iter2; 
	}
	[[nodiscard]] bool contains(K& key) { return find(key) != keys.end(); }
	[[nodiscard]] std::vector<K>::iterator find(K& key) { return std::ranges::find(keys, key); }
	[[nodiscard]] std::vector<std::pair<K, V>> begin() { return data.begin(); }
	[[nodiscard]] std::vector<std::pair<K, V>> end() { return data.end(); }
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
