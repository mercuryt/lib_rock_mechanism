#pragma once

#include "limitedSet.h"
template<typename T, uint capacity>
LimitedSet<T, capacity>::LimitedSet(const std::initializer_list<T>& i) { for(const T& value : i) insert(value); }
template<typename T, uint capacity>
Json LimitedSet<T, capacity>::toJson() const
{
	Json output;
	auto iter = m_data.begin();
	const auto end = iter + m_size;
	for(; iter != end; ++iter)
	{
		const T& value = *iter;
		if constexpr(std::is_pointer<T>())
			output.push_back(std::to_string((uintptr_t)value));
		else
			output.push_back((Json)value);
	}
	return output;
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::fromJson(const Json& data)
{
	if constexpr(HasFromJsonMethod<T>)
	{
		for(const Json& valueData : data)
			insert(valueData);
	}
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insert(const T& value) { assert(!contains(value)); insertNonunique(value); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::maybeInsert(const T& value) { if(!contains(value)) insertNonunique(value); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insert(std::array<T, capacity>::const_iterator begin, const std::array<T, capacity>::const_iterator& end)
{
	for(; begin != end; ++begin)
		insert(*begin);
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insert(LimitedSet<T, capacity>::iterator begin, const LimitedSet<T, capacity>::iterator& end)
{
	for(; begin != end; ++begin)
		insert(*begin);
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insertNonunique(const T& value) { assert(m_size < capacity); m_data[m_size] = value; ++m_size; }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insertFrontNonunique(const T& value)
{
	assert(m_size < capacity);
	const auto copy = m_data;
	std::ranges::copy(copy, m_data.begin() + 1);
	m_data[0] = value;
	++m_size;
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::maybeInsertAll(const LimitedSet<T, capacity>& other) { maybeInsertAll(other.begin(), other.end()); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insertAll(const This& other) { insertAll(other.begin(), other.end()); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::insertAllNonunique(const LimitedSet<T, capacity>& other) { for(const T& value : other) insertNonunique(value); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::erase(const T& value)
{
	auto found = std::ranges::find(m_data, value);
	assert(*found != m_data[m_size - 1]);
	erase(found);
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::maybeErase(const T& value)
{
	auto found = std::ranges::find(m_data, value);
	if(*found != m_data[m_size - 1])
		erase(found);
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::erase(LimitedSet<T, capacity>::iterator iter)
{
	assert(iter != end());
	assert(!m_data.empty());
	assert(begin() + m_size >= iter);
	*iter = m_data[m_size - 1];
	--m_size;
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::eraseAll(LimitedSet<T, capacity>& other)
{
	const auto copy = std::move(m_data);
	m_size = 0;
	const auto begin = copy.begin();
	const auto end = begin + m_size;
	for(auto iter = begin; iter != end; ++iter)
		if(!other.contains(*iter))
			insert(*iter);
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::eraseIndex(const uint& index) { erase(begin() + index); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::popBack() { --m_size; }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::clear() { m_size = 0; }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::swap(LimitedSet<T, capacity>& other) { m_data.swap(other.m_data); }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::sort()
{
	const auto begin = m_data.begin();
	const auto end = begin + m_size;
	if constexpr(HasHashMethod<T>)
		std::ranges::sort(begin, end);
	else if constexpr(HasGetMethod<T>)
		std::ranges::sort(begin, end, [&](const auto& a, const auto& b) { return a.get() < b.get(); });
	else
	{
		assert(false);
		std::unreachable();
	}
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::update(const T& oldValue, const T& newValue) { assert(!contains(newValue)); assert(contains(oldValue)); (*find(oldValue)) = newValue;}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::updateIfExists(const T& oldValue, const T& newValue) { assert(!contains(newValue)); auto found = find(oldValue); if(found != begin() + m_size) (*found) = newValue; }
template<typename T, uint capacity>
void LimitedSet<T, capacity>::updateIfExistsAndNewValueDoesNot(const T& oldValue, const T& newValue)
{
	auto found = find(oldValue);
	if(found != begin() + m_size && !contains(newValue))
		(*found) = newValue;
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::makeUnique()
{
	const auto begin = m_data.begin();
	const auto end = begin + m_size;
	if constexpr(HasHashMethod<T>)
		std::ranges::sort(begin, end);
	else if constexpr(HasGetMethod<T>)
		std::ranges::sort(begin, end, [&](const auto& a, const auto& b) { return a.get() < b.get(); });
	else
	{
		assert(false);
		std::unreachable();
	}
	const auto newEnd = std::ranges::unique(m_data).begin();
	m_size = newEnd - m_data.begin();
}
template<typename T, uint capacity>
void LimitedSet<T, capacity>::removeDuplicatesAndValue(const T& value)
{
	const auto begin = m_data.begin();
	const auto end = begin + m_size;
	std::vector<T> seen = {value};
	const auto newEnd = std::remove_if(begin, end, [&](const T& elem)
	{
		if(std::ranges::contains(seen, elem))
			return true;
		seen.push_back(elem);
		return false;
	});
	m_size = newEnd - begin;
}
template<typename T, uint capacity>
bool LimitedSet<T, capacity>::operator==(const LimitedSet<T, capacity>& other) { return &other == this; }
template<typename T, uint capacity>
const T& LimitedSet<T, capacity>::operator[](const uint& index) const { assert(index < m_size); return m_data[index]; }
template<typename T, uint capacity>
T& LimitedSet<T, capacity>::operator[](const uint& index) { assert(index < m_size); return m_data[index]; }
template<typename T, uint capacity>
bool LimitedSet<T, capacity>::contains(const T& value) const
{
	const auto begin = m_data.begin();
	return std::ranges::contains(begin, begin + m_size, value);
}
template<typename T, uint capacity>
uint LimitedSet<T, capacity>::indexOf(const T& value) const
{
	assert(contains(value));
	const auto begin = m_data.begin();
	return std::distance(begin, std::ranges::find(begin, begin + m_size, value));
}
template<typename T, uint capacity>
T& LimitedSet<T, capacity>::front() { return m_data.front(); }
template<typename T, uint capacity>
const T& LimitedSet<T, capacity>::front() const { return m_data.front(); }
template<typename T, uint capacity>
T& LimitedSet<T, capacity>::back() { return m_data[m_size - 1]; }
template<typename T, uint capacity>
const T& LimitedSet<T, capacity>::back() const { return m_data[m_size - 1]; }
template<typename T, uint capacity>
bool LimitedSet<T, capacity>::empty() const { return m_data.empty(); }
template<typename T, uint capacity>
uint LimitedSet<T, capacity>::size() const { return m_size; }
template<typename T, uint capacity>
LimitedSet<T, capacity>::iterator LimitedSet<T, capacity>::begin() { return {*this, 0}; }
template<typename T, uint capacity>
LimitedSet<T, capacity>::iterator LimitedSet<T, capacity>::end() { return {*this, size()}; }
template<typename T, uint capacity>
LimitedSet<T, capacity>::const_iterator LimitedSet<T, capacity>::begin() const { return LimitedSet<T, capacity>::const_iterator(*this, 0); }
template<typename T, uint capacity>
LimitedSet<T, capacity>::const_iterator LimitedSet<T, capacity>::end() const { return LimitedSet<T, capacity>::const_iterator(*this, size()); }
template<typename T, uint capacity>
LimitedSet<T, capacity>::iterator LimitedSet<T, capacity>::find(const T& value) { const auto begin = m_data.begin(); return std::ranges::find(begin, begin + m_size, value); }
template<typename T, uint capacity>
LimitedSet<T, capacity>::const_iterator LimitedSet<T, capacity>::find(const T& value) const { const auto begin = m_data.begin(); return std::ranges::find(begin, begin + m_size, value); }
template<typename T, uint capacity>
bool LimitedSet<T, capacity>::isUnique() const { auto copy = *this; copy.makeUnique(); return size() == copy.size(); }
template<typename T, uint capacity>
uint LimitedSet<T, capacity>::findLastIndex(const T& value) const
{
	const auto it = std::ranges::find(m_data.rbegin() + capacity - m_size, m_data.rend(), value);
	assert(it != m_data.rend());
	return std::distance(m_data.begin(), it.base()) - 1;
}
template<typename T, uint capacity>
std::string LimitedSet<T, capacity>::toString() const
{
	std::string output;
	const auto begin = m_data.begin();
	const auto end = begin + m_size;
	for(auto iter = begin; iter != end; ++iter)
	{
		const T& value = *iter;
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