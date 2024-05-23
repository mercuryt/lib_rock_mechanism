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
template<typename K, typename V>
class SmallMap
{
	std::vector<std::pair<K, V>> data;
public:
	void insert(K& key, V& value) { assert(!contains(key)); data.emplace_back(key, value); }
	void erase(K& key)
	{
		auto iter = find(key);
		std::swap(iter, data.back());
		data.pop_back();
	}
	void clear() { data.clear(); }
	[[nodiscard]] V& at(K& key) { assert(contains(key)); return *find(key); }
	[[nodiscard]] bool contains(K& key) { return find(key) != data.end(); }
	[[nodiscard]] std::vector<std::pair<K, V>>::iterator find(K& key) 
	{
		return std::ranges::find_if(data, key, std::pair<K, V>::first);
	}
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
