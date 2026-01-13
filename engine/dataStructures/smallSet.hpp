#pragma once

#include "smallSet.h"
#include "../concepts.h"
template<typename T>
SmallSet<T>::SmallSet(int capacity) { reserve(capacity); };
template<typename T>
SmallSet<T>::SmallSet(std::initializer_list<T> i) : m_data(i) { }
template<typename T>
Json SmallSet<T>::toJson() const
{
	Json output;
	for(const T& value : m_data)
		if constexpr(std::is_pointer<T>())
			output.push_back(std::to_string((uintptr_t)value));
		else
			output.push_back((Json)value);
	return output;
}
template<typename T>
void SmallSet<T>::fromJson(const Json& data)
{
	if constexpr(HasFromJsonMethod<T>)
	{
		for(const Json& valueData : data)
			m_data.emplace_back(valueData);
	}
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<typename T>
void SmallSet<T>::insert(const T& value) { assert(!contains(value)); m_data.push_back(value); }
template<typename T>
void SmallSet<T>::maybeInsert(const T& value) { if(!contains(value)) m_data.push_back(value); }
template<typename T>
void SmallSet<T>::insert(std::vector<T>::const_iterator begin, const std::vector<T>::const_iterator& end)
{
	for(; begin != end; ++begin)
		insert(*begin);
}
template<typename T>
void SmallSet<T>::insert(SmallSet<T>::iterator begin, const SmallSet<T>::iterator& end)
{
	for(; begin != end; ++begin)
		insert(*begin);
}
template<typename T>
void SmallSet<T>::insertNonunique(const T& value) { m_data.push_back(value); }
template<typename T>
void SmallSet<T>::insertFrontNonunique(const T& value) { const auto copy = m_data; m_data.resize(m_data.size() + 1); std::ranges::copy(copy, m_data.begin() + 1); m_data[0] = value;}
template<typename T>
void SmallSet<T>::maybeInsertAll(const SmallSet<T>& other)
{
	maybeInsertAll(other.begin(), other.end());
}
template<typename T>
void SmallSet<T>::insertAll(const This& other)
{
	insertAll(other.begin(), other.end());
}
template<typename T>
void SmallSet<T>::insertAllNonunique(const SmallSet<T>& other)
{
	for(const T& value : other)
		insertNonunique(value);
}
template<typename T>
void SmallSet<T>::erase(const T& value)
{
	auto found = std::ranges::find(m_data, value);
	assert(found != m_data.end());
	*found = m_data.back();
	m_data.pop_back();
}
template<typename T>
void SmallSet<T>::maybeErase(const T& value)
{
	auto found = std::ranges::find(m_data, value);
	if(found != m_data.end())
	{
		*found = m_data.back();
		m_data.pop_back();
	}
}
template<typename T>
void SmallSet<T>::erase(SmallSet<T>::iterator iter)
{
	assert(iter != end());
	assert(!m_data.empty());
	*iter = m_data.back();
	m_data.pop_back();
}
template<typename T>
void SmallSet<T>::eraseAll(SmallSet<T>& other)
{
	std::erase_if(m_data, [&](const T& value){ return std::ranges::contains(other.m_data, value); });
}
template<typename T>
void SmallSet<T>::maybeEraseAllWhereBothSetsAreSorted(const This& other)
{
	auto new_end = std::remove_if(m_data.begin(), m_data.end(),
			[&](const T& x) {
				// Check if element x exists in the second sorted range
				return std::binary_search(other.m_data.begin(), other.m_data.end(), x);
			}
		);
    // Erase the elements at the end of the vector
    m_data.erase(new_end, m_data.end());
}
template<typename T>
void SmallSet<T>::eraseIndex(const int& index)
{
	erase(m_data.begin() + index);
}
template<typename T>
void SmallSet<T>::popBack() { m_data.pop_back(); }
template<typename T>
void SmallSet<T>::clear() { m_data.clear(); }
template<typename T>
void SmallSet<T>::swap(SmallSet<T>& other) { m_data.swap(other.m_data); }
template<typename T>
void SmallSet<T>::sort()
{
	if constexpr(Sortable<T>)
		std::ranges::sort(m_data);
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<typename T>
void SmallSet<T>::update(const T& oldValue, const T& newValue) { assert(!contains(newValue)); assert(contains(oldValue)); (*find(oldValue)) = newValue;}
template<typename T>
void SmallSet<T>::updateIfExists(const T& oldValue, const T& newValue) { assert(!contains(newValue)); auto found = find(oldValue); if(found != m_data.end()) (*found) = newValue; }
template<typename T>
void SmallSet<T>::updateIfExistsAndNewValueDoesNot(const T& oldValue, const T& newValue) { auto found = find(oldValue); if(found != m_data.end() && !contains(newValue)) (*found) = newValue; }
template<typename T>
void SmallSet<T>::makeUnique()
{
	sort();
	m_data.erase(std::ranges::unique(m_data).begin(), m_data.end());
}
template<typename T>
void SmallSet<T>::removeDuplicatesAndValue(const T& value)
{
	std::vector<T> seen = {value};
	m_data.erase(std::remove_if(m_data.begin(), m_data.end(), [&](const T& elem)
	{
		if(std::ranges::contains(seen, elem))
			return true;
		seen.push_back(elem);
		return false;
	}), m_data.end());
}
template<typename T>
void SmallSet<T>::reserve(int size) { m_data.reserve(size); }
template<typename T>
void SmallSet<T>::resize(int size) { m_data.resize(size); }
template<typename T>
bool SmallSet<T>::operator==(const SmallSet<T>& other) { return &other == this; }
template<typename T>
const T& SmallSet<T>::operator[](const int& index) const { return m_data[index]; }
template<typename T>
T& SmallSet<T>::operator[](const int& index) { return m_data[index]; }
template<typename T>
bool SmallSet<T>::contains(const T& value) const { return std::ranges::find(m_data, value) != m_data.end(); }
template<typename T>
bool SmallSet<T>::containsAny(const This& other) const
{
	// TODO: can this be rewritten as a single pass?
	for(const T& item : m_data)
		if(other.contains(item))
			return true;
	return false;
}
template<typename T>
int SmallSet<T>::indexOf(const T& value) const { assert(contains(value)); return std::distance(m_data.begin(), std::ranges::find(m_data, value)); }
template<typename T>
T& SmallSet<T>::front() { return m_data.front(); }
template<typename T>
const T& SmallSet<T>::front() const { return m_data.front(); }
template<typename T>
T& SmallSet<T>::back() { return m_data.back(); }
template<typename T>
const T& SmallSet<T>::back() const { return m_data.back(); }
template<typename T>
bool SmallSet<T>::empty() const { return m_data.empty(); }
template<typename T>
int SmallSet<T>::size() const { return m_data.size(); }
template<typename T>
SmallSet<T>::iterator SmallSet<T>::begin() { return {*this, 0}; }
template<typename T>
SmallSet<T>::iterator SmallSet<T>::end() { return {*this, size()}; }
template<typename T>
SmallSet<T>::const_iterator SmallSet<T>::begin() const { return SmallSet<T>::const_iterator(*this, 0); }
template<typename T>
SmallSet<T>::const_iterator SmallSet<T>::end() const { return SmallSet<T>::const_iterator(*this, size()); }
template<typename T>
std::vector<T>& SmallSet<T>::getVector() { return m_data; }
template<typename T>
const std::vector<T>& SmallSet<T>::getVector() const { return m_data; }
template<typename T>
SmallSet<T>::iterator SmallSet<T>::find(const T& value) { return std::ranges::find(m_data, value); }
template<typename T>
SmallSet<T>::const_iterator SmallSet<T>::find(const T& value) const { return std::ranges::find(m_data, value); }
template<typename T>
bool SmallSet<T>::isUnique() const { auto copy = *this; copy.makeUnique(); return size() == copy.size(); }
template<typename T>
int SmallSet<T>::findLastIndex(const T& value) const
{
	const auto it = std::ranges::find(m_data.rbegin(), m_data.rend(), value);
	assert(it != m_data.rend());
	return std::distance(m_data.begin(), it.base()) - 1;
}
template<typename T>
int SmallSet<T>::maybeFindLastIndex(const T& value) const
{
	const auto it = std::ranges::find(m_data.rbegin(), m_data.rend(), value);
	if(it == m_data.rend())
		return -1;
	return std::distance(m_data.begin(), it.base()) - 1;
}
template<typename T>
std::pair<SmallSet<T>, SmallSet<T>> SmallSet<T>::getDeltaPair(SmallSet<T>& other)
{
	// First set is items to remove, second is items to insert, when transitioning from this to other.
	std::pair<SmallSet<T>, SmallSet<T>> output;
	other.sort();
	sort();
	std::set_difference(m_data.begin(), m_data.end(), other.m_data.begin(), other.m_data.end(), std::back_inserter(output.first.m_data));
	std::set_difference(other.m_data.begin(), other.m_data.end(), m_data.begin(), m_data.end(), std::back_inserter(output.second.m_data));
	return output;
}
template<typename T>
std::string SmallSet<T>::toString() const
{
	std::string output;
	for(const T& value : m_data)
	{
		if constexpr(HasToStringMethod<T>)
			output += value.toString() + ", ";
		else if constexpr(std::is_pointer<T>())
			output += std::to_string((uintptr_t)value) + ", ";
		else if constexpr(Numeric<T>)
			output += std::to_string(value) + ", ";
		else
			assert(false);
	}
	return output;
}
// SmallSetStable
template<typename T>
SmallSetStable<T>::SmallSetStable(std::initializer_list<T> i) : m_data(i) { }
template<typename T>
SmallSetStable<T>::SmallSetStable(const Json& data) { fromJson(data); }
template<typename T>
void SmallSetStable<T>::fromJson(const Json& data)
{
	for(const Json& valueData : data)
		m_data.emplace_back(std::make_unique<T>(valueData));
}
template<typename T>
Json SmallSetStable<T>::toJson() const
{
	Json output = Json::array();
	for(const std::unique_ptr<T>& value : m_data)
		output.push_back(*value);
	return output;
}
template<typename T>
void SmallSetStable<T>::insert(std::unique_ptr<T>&& value) { assert(!contains(value)); m_data.push_back(std::move(value)); }
template<typename T>
void SmallSetStable<T>::maybeInsert(std::unique_ptr<T>&& value) { if(!contains(*value)) m_data.push_back(std::move(value)); }
template<typename T>
void SmallSetStable<T>::insert(SmallSetStable<T>::iterator begin, SmallSetStable<T>::iterator end)
{
	for(; begin != end; ++begin)
		insert(*begin);
}
template<typename T>
void SmallSetStable<T>::erase(const std::unique_ptr<T>& value)
{
	auto found = std::ranges::find(m_data, value);
	assert(found != m_data.end());
	*found = std::move(m_data.back());
	m_data.pop_back();
}
template<typename T>
void SmallSetStable<T>::maybeErase(const T& value)
{
	auto found = std::ranges::find(m_data, value);
	if(found != m_data.end())
	{
		*found = std::move(m_data.back());
		m_data.pop_back();
	}
}
template<typename T>
void SmallSetStable<T>::erase(const SmallSetStable<T>::iterator& iter)
{
	assert(iter != end());
	assert(!m_data.empty());
	int index = std::distance(m_data.begin(), iter.getIter());
	m_data[index] = std::move(m_data.back());
	m_data.pop_back();
}
template<typename T>
void SmallSetStable<T>::erase(const T& value)
{
	auto iter = find(value);
	erase(iter);
}
template<typename T>
void SmallSetStable<T>::clear() { m_data.clear(); }
template<typename T>
void SmallSetStable<T>::swap(SmallSetStable<T>& other) { m_data.swap(other.m_data); }
template<typename T>
void SmallSetStable<T>::popBack() { m_data.pop_back(); }
template<typename T>
void SmallSetStable<T>::makeUnique() { std::ranges::sort(m_data); m_data.erase(std::ranges::unique(m_data).begin(), m_data.end()); }
template<typename T>
bool SmallSetStable<T>::contains(const std::unique_ptr<T>& value) const { return std::ranges::find(m_data, value) != m_data.end(); }
template<typename T>
T& SmallSetStable<T>::front() { return *m_data.front(); }
template<typename T>
const T& SmallSetStable<T>::front() const { return *m_data.front(); }
template<typename T>
T& SmallSetStable<T>::back() { return *m_data.back(); }
template<typename T>
const T& SmallSetStable<T>::back() const { return *m_data.back(); }
template<typename T>
bool SmallSetStable<T>::empty() const { return m_data.empty(); }
template<typename T>
int SmallSetStable<T>::size() const { return m_data.size(); }
template<typename T>
SmallSetStable<T>::iterator SmallSetStable<T>::begin() { return {*this, 0}; }
template<typename T>
SmallSetStable<T>::iterator SmallSetStable<T>::end() { return {*this, size()}; }
template<typename T>
SmallSetStable<T>::const_iterator SmallSetStable<T>::begin() const { return SmallSetStable<T>::const_iterator(*this, 0); }
template<typename T>
SmallSetStable<T>::const_iterator SmallSetStable<T>::end() const { return SmallSetStable<T>::const_iterator(*this, size()); }
template<typename T>
SmallSetStable<T>::iterator SmallSetStable<T>::find(const T& value) { return std::ranges::find_if(m_data, [&](std::unique_ptr<T>& d){ return *d == value; }); }
template<typename T>
SmallSetStable<T>::const_iterator SmallSetStable<T>::find(const T& value) const { return const_cast<SmallSetStable&>(*this).find(value); }