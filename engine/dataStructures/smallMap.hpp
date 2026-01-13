#pragma once
#include "smallMap.h"
#include "smallSet.h"
template<typename K, MoveConstructible V>
SmallMap<K,V>::SmallMap(const std::initializer_list<Pair>& i)
{
	if constexpr (std::is_copy_assignable_v<Pair>)
		m_data = i;
	else
		assert(false);
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::insert(const K& key, const V& value)
{
	assert(!contains(key));
	insertNonUnique(key, value);
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::insert(const K& key, V&& value)
{
	assert(!contains(key));
	m_data.emplace_back(key, std::move(value));
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::maybeInsert(const K& key, const V& value)
{
	if(!contains(key)) insert(key, value);
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::maybeInsert(const K& key, V&& value) { if(!contains(key)) insert(key, std::forward<V>(value)); }
template<typename K, MoveConstructible V>
void SmallMap<K,V>::insertNonUnique(const K& key, const V& value)
{
	if constexpr (std::is_copy_constructible_v<V>)
		m_data.emplace_back(key, value);
	else
		assert(false);
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::erase(const K& key)
{
	erase(std::ranges::find(m_data, key, &Pair::first));
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::erase(iterator& iter)
{
	assert(iter != m_data.end());
	if(iter != m_data.end() - 1)
	{
		iter->first = m_data.back().first;
		iter->second = std::move(m_data.back().second);
	}
	m_data.pop_back();
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::erase(iterator&& iter) { erase(iter); }
template<typename K, MoveConstructible V>
void SmallMap<K,V>::maybeErase(const K& key)
{
	typename Data::iterator iter = std::ranges::find(m_data, key, &Pair::first);
	if(iter != m_data.end())
		m_data.erase(iter);
}
template<typename K, MoveConstructible V>
bool SmallMap<K,V>::maybeEraseNotify(const K& key)
{
	typename Data::iterator iter = std::ranges::find(m_data, key, &Pair::first);
	if(iter != m_data.end())
	{
		m_data.erase(iter);
		return true;
	}
	return false;
}
template<typename K, MoveConstructible V>
void SmallMap<K,V>::popBack() { m_data.pop_back(); }
template<typename K, MoveConstructible V>
void SmallMap<K,V>::clear() { m_data.clear(); }
template<typename K, MoveConstructible V>
void SmallMap<K,V>::sort() { std::ranges::sort(m_data, {}, &std::pair<K, V>::first); }
template<typename K, MoveConstructible V>
void SmallMap<K,V>::swap(This& other) { m_data.swap(other.m_data); }
template<typename K, MoveConstructible V>
void SmallMap<K,V>::updateKey(const K& oldKey, const K& newKey)
{
	for(auto& [key, value] : *this)
		if(key == oldKey)
		{
			key = newKey;
			return;
		}
}
template<typename K, MoveConstructible V>
int SmallMap<K,V>::size() const { return m_data.size(); }
template<typename K, MoveConstructible V>
bool SmallMap<K,V>::empty() const { return m_data.empty(); }
template<typename K, MoveConstructible V>
bool SmallMap<K,V>::contains(const K& key) const { return std::ranges::find(m_data, key, &Pair::first) != m_data.end(); }
template<typename K, MoveConstructible V>
bool SmallMap<K,V>::containsAny(const SmallSet<K>& keys) const { for(const K& key : keys) if(contains(key)) return true; return false; }
template<typename K, MoveConstructible V>
SmallMap<K,V>::Pair& SmallMap<K,V>::front() { return m_data.front(); }
template<typename K, MoveConstructible V>
const SmallMap<K,V>::Pair& SmallMap<K,V>::front() const { return m_data.front(); }
template<typename K, MoveConstructible V>
SmallMap<K,V>::Pair& SmallMap<K,V>::back() { return m_data.back(); }
template<typename K, MoveConstructible V>
const SmallMap<K,V>::Pair& SmallMap<K,V>::back() const { return m_data.back(); }
template<typename K, MoveConstructible V>
V& SmallMap<K,V>::operator[](const K& key)
{
	auto iter = std::ranges::find(m_data, key, &Pair::first);
	assert(iter != m_data.end());
	return iter->second;
}
template<typename K, MoveConstructible V>
const V& SmallMap<K,V>::operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
template<typename K, MoveConstructible V>
V& SmallMap<K,V>::getOrInsert(const K& key, V&& value)
{
	auto iter = std::ranges::find(m_data, key, &Pair::first);
	if(iter == m_data.end())
	{
		insert(key, std::move(value));
		return m_data.back().second;
	}
	return iter->second;
}
template<typename K, MoveConstructible V>
V& SmallMap<K,V>::getOrCreate(const K& key)
{
	auto iter = std::ranges::find(m_data, key, &Pair::first);
	if(iter == m_data.end())
	{
		insert(key, V{});
		return m_data.back().second;
	}
	return iter->second;
}
template<typename K, MoveConstructible V>
SmallSet<K> SmallMap<K,V>::keys() const
{
	SmallSet<K> output;
	output.reserve(size());
	for(const auto& pair : m_data)
		output.insert(pair.first);
	return output;
}
template<typename K, MoveConstructible V>
SmallMap<K,V>::iterator SmallMap<K,V>::find(const K& key) { return std::ranges::find(m_data, key, &Pair::first); }
template<typename K, MoveConstructible V>
SmallMap<K,V>::const_iterator SmallMap<K,V>::find(const K& key) const { return std::ranges::find(m_data, key, &Pair::first); }
template<typename K, MoveConstructible V>
SmallMap<K,V>::iterator SmallMap<K,V>::begin() { return {*this, 0}; }
template<typename K, MoveConstructible V>
SmallMap<K,V>::iterator SmallMap<K,V>::end() { return {*this, size()}; }
template<typename K, MoveConstructible V>
SmallMap<K,V>::const_iterator SmallMap<K,V>::begin() const { return {*this, 0}; }
template<typename K, MoveConstructible V>
SmallMap<K,V>::const_iterator SmallMap<K,V>::end() const { return {*this, size()}; }
// SmallMapStable
// TODO: Why are we not using std::ranges::find here?
template<typename K, typename V>
SmallMapStable<K,V>::Data::iterator SmallMapStable<K,V>::findData(const K& key) { typename Data::iterator iter = m_data.begin(); const typename Data::iterator e = m_data.end(); while(iter != e) if(iter->first == key) return iter; else ++iter; return iter;}
template<typename K, typename V>
SmallMapStable<K,V>::Data::const_iterator SmallMapStable<K,V>::findData(const K& key) const { typename Data::const_iterator iter = m_data.begin(); const typename Data::const_iterator e = m_data.end(); while(iter != e) if(iter->first == key) return iter; else ++iter; return iter;}
template<typename K, typename V>
SmallMapStable<K,V>::SmallMapStable(const Json& data) { fromJson(data); }
template<typename K, typename V>
void SmallMapStable<K,V>::fromJson(const Json& data)
{
	if constexpr (HasFromJsonMethod<V>)
		for(const Json& pair : data)
			m_data.emplace(pair[0], std::unique_ptr<V>(pair[1]));
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<typename K, typename V>
Json SmallMapStable<K,V>::toJson() const
{
	if constexpr (HasToJsonMethod<V>)
	{
		Json output = Json::array();
		for(const auto& [key, value] : m_data)
		{
			Json jsonValue;
			to_json(jsonValue, *value.get());
			output.push_back({key, jsonValue});
		}
		return output;
	}
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<typename K, typename V>
auto SmallMapStable<K,V>::insert(const K& key, std::unique_ptr<V>&& value) -> V& { m_data.insert(key, std::move(value)); return *m_data[key]; }
template<typename K, typename V>
void SmallMapStable<K,V>::erase(const K& key) { m_data.erase(key); }
template<typename K, typename V>
void SmallMapStable<K,V>::erase(iterator& iter) { m_data.erase(iter.get()); }
template<typename K, typename V>
void SmallMapStable<K,V>::clear() { m_data.clear(); }
template<typename K, typename V>
void SmallMapStable<K,V>::swap(This& other) { m_data.swap(other.m_data); }
template<typename K, typename V>
int SmallMapStable<K,V>::size() const { return m_data.size(); }
template<typename K, typename V>
bool SmallMapStable<K,V>::empty() const { return m_data.empty(); }
template<typename K, typename V>
bool SmallMapStable<K,V>::contains(const K& key) const { return m_data.contains(key); }
template<typename K, typename V>
SmallMapStable<K,V>::Pair& SmallMapStable<K,V>::front() { return m_data.front(); }
template<typename K, typename V>
const SmallMapStable<K,V>::Pair& SmallMapStable<K,V>::front() const { return m_data.front(); }
template<typename K, typename V>
SmallMapStable<K,V>::Pair& SmallMapStable<K,V>::back() { return m_data.back(); }
template<typename K, typename V>
const SmallMapStable<K,V>::Pair& SmallMapStable<K,V>::back() const { return m_data.back(); }
template<typename K, typename V>
auto SmallMapStable<K,V>::operator[](const K& key) -> V& { return *m_data[key].get(); }
template<typename K, typename V>
auto SmallMapStable<K,V>::operator[](const K& key) const -> const V& { return const_cast<This&>(*this)[key]; }
template<typename K, typename V>
SmallMapStable<K,V>::iterator SmallMapStable<K,V>::find(const K& key) { return m_data.find(key); }
template<typename K, typename V>
SmallMapStable<K,V>::const_iterator SmallMapStable<K,V>::find(const K& key) const { return findData(key); }
template<typename K, typename V>
SmallMapStable<K,V>::iterator SmallMapStable<K,V>::begin() { return {*this, 0} ; }
template<typename K, typename V>
SmallMapStable<K,V>::iterator SmallMapStable<K,V>::end() { return {*this, size()}; }
template<typename K, typename V>
SmallMapStable<K,V>::const_iterator SmallMapStable<K,V>::begin() const { return {*this, 0}; }
template<typename K, typename V>
SmallMapStable<K,V>::const_iterator SmallMapStable<K,V>::end() const { return {*this, size()}; }
template<typename K, typename V>
auto SmallMapStable<K,V>::getOrCreate(const K& key) -> V&
{
	typename Data::iterator iter = findData(key);
	if(iter == m_data.end())
		return *m_data.getOrCreate(key);
	return *(iter->second);
}