#pragma once
/*
 * To be used for short seqences of data, not long enough for binary search to make sense.
 * Not pointer stable.
 */
#include "json.h"
#include <algorithm>
#include <vector>
#include <cassert>

template<typename T>
class SmallSet
{
	using This = SmallSet<T>;
	std::vector<T> m_data;
public:
	class iterator;
	class const_iterator;
	SmallSet() = default;
	SmallSet(uint capacity) { reserve(capacity); };
	SmallSet(std::initializer_list<T> i) : m_data(i) { }
	SmallSet(const This& other) = default;
	SmallSet(This&& other) noexcept = default;
	This& operator=(const This& other) = default;
	This& operator=(This&& other) noexcept = default;
	[[nodiscard]] Json toJson() const { return m_data; }
	void fromJson(const Json& data)
	{
		for(const Json& valueData : data)
			m_data.emplace_back(valueData);
	}
	void insert(const T& value) { assert(!contains(value)); m_data.push_back(value); }
	void maybeInsert(const T& value) { if(!contains(value)) m_data.push_back(value); }
	void insert(std::vector<T>::const_iterator begin, const std::vector<T>::const_iterator& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void insert(This::iterator begin, const This::iterator& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void insertNonunique(const T& value) { m_data.push_back(value); }
	void insertFrontNonunique(const T& value) { const auto copy = m_data; m_data.resize(m_data.size() + 1); std::ranges::copy(copy, m_data.begin() + 1); m_data[0] = value;}
	void maybeInsertAll(const This& other)
	{
		maybeInsertAll(other.begin(), other.end());
	}
	template<typename Iterator>
	void maybeInsertAll(Iterator begin, const Iterator& end)
	{
		for(; begin != end; ++begin)
			insertNonunique(*begin);
		makeUnique();
	}
	void maybeInsertAll(const auto& source)
	{
		maybeInsertAll(source.begin(), source.end());
	}
	void insertAll(const This& other)
	{
		insertAll(other.begin(), other.end());
	}
	void insertAllNonunique(const This& other)
	{
		for(const T& value : other)
			insertNonunique(value);
	}
	template<typename Iterator>
	void insertAll(Iterator begin, const Iterator& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void erase(const T& value)
	{
		auto found = std::ranges::find(m_data, value);
		assert(found != m_data.end());
		*found = m_data.back();
		m_data.pop_back();
	}
	void maybeErase(const T& value)
	{
		auto found = std::ranges::find(m_data, value);
		if(found != m_data.end())
		{
			*found = m_data.back();
			m_data.pop_back();
		}
	}
	void erase(This::iterator iter)
	{
		assert(iter != end());
		assert(!m_data.empty());
		*iter = m_data.back();
		m_data.pop_back();
	}
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void eraseAll(This& other)
	{
		other.sort();
		std::erase_if(m_data, [&](const T& value){ return std::ranges::binary_search(other.m_data, value); });
	}
	template<typename Iterator>
	void maybeEraseAll(Iterator begin, const Iterator& end)
	{
		for(; begin != end; ++begin)
			maybeErase(*begin);
	}
	void eraseIndex(const uint& index)
	{
		erase(m_data.begin() + index);
	}
	void popBack() { m_data.pop_back(); }
	void clear() { m_data.clear(); }
	template<typename ...Args>
	void emplace(Args&& ...args)
	{
		// Create first and then assert uniqueness.
		m_data.emplace_back(T(std::forward<Args>(args)...));
		assert(std::ranges::count(m_data, m_data.back()) == 1);
	}
	template<typename ...Args>
	void maybeEmplace(Args&& ...args)
	{
		// Create first and then maybe remove.
		m_data.emplace_back(T(std::forward<Args>(args)...));
		if(std::ranges::count(m_data, m_data.back()) != 1)
			m_data.pop_back();
	}
	void swap(This& other) { m_data.swap(other.m_data); }
	template<typename Predicate>
	void sort(Predicate&& predicate) { std::ranges::sort(m_data, predicate); }
	void sort() { std::ranges::sort(m_data); }
	void update(const T& oldValue, const T& newValue) { assert(!contains(newValue)); assert(contains(oldValue)); (*find(oldValue)) = newValue;}
	void updateIfExists(const T& oldValue, const T& newValue) { assert(!contains(newValue)); auto found = find(oldValue); if(found != m_data.end()) (*found) = newValue; }
	void updateIfExistsAndNewValueDoesNot(const T& oldValue, const T& newValue) { auto found = find(oldValue); if(found != m_data.end() && !contains(newValue)) (*found) = newValue; }
	void makeUnique() { std::ranges::sort(m_data); m_data.erase(std::ranges::unique(m_data).begin(), m_data.end()); }
	void removeDuplicatesAndValue(const T& value)
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
	void reserve(uint size) { m_data.reserve(size); }
	void resize(uint size) { m_data.resize(size); }
	[[nodiscard]] bool operator==(const This& other) { return &other == this; }
	template<typename Predicate>
	[[nodiscard]] uint countIf(Predicate&& predicate) const { return std::ranges::count_if(m_data, predicate); }
	[[nodiscard]] const T& operator[](const uint& index) const { return m_data[index]; }
	[[nodiscard]] T& operator[](const uint& index) { return m_data[index]; }
	[[nodiscard]] bool contains(const T& value) const { return std::ranges::find(m_data, value) != m_data.end(); }
	template<typename Predicate>
	[[nodiscard]] bool containsAny(Predicate&& predicate) const { return std::ranges::find_if(m_data, predicate) != m_data.end(); }
	[[nodiscard]] uint indexOf(const T& value) const { assert(contains(value)); return std::distance(m_data.begin(), find(value)); }
	[[nodiscard]] T& front() { return m_data.front(); }
	[[nodiscard]] const T& front() const { return m_data.front(); }
	[[nodiscard]] T& back() { return m_data.back(); }
	[[nodiscard]] const T& back() const { return m_data.back(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] This::iterator begin() { return {*this, 0}; }
	[[nodiscard]] This::iterator end() { return {*this, size()}; }
	[[nodiscard]] This::const_iterator begin() const { return This::const_iterator(*this, 0); }
	[[nodiscard]] This::const_iterator end() const { return This::const_iterator(*this, size()); }
	[[nodiscard]] std::vector<T>& getVector() { return m_data; }
	[[nodiscard]] const std::vector<T>& getVector() const { return m_data; }
	[[nodiscard]] This::iterator find(const T& value) { return std::ranges::find(m_data, value); }
	[[nodiscard]] This::const_iterator find(const T& value) const { return std::ranges::find(m_data, value); }
	[[nodiscard]] uint findLastIndex(const T& value) const
	{
		const auto it = std::ranges::find(m_data.rbegin(), m_data.rend(), value);
		assert(it != m_data.rend());
		return std::distance(m_data.begin(), it.base()) - 1;
	}
	template<typename Predicate>
	[[nodiscard]] This::iterator findIf(Predicate&& predicate) { return std::ranges::find_if(m_data, predicate); }
	template<typename Predicate>
	[[nodiscard]] This::const_iterator findIf(Predicate&& predicate) const { return std::ranges::find_if(m_data, predicate); }
	template<typename Predicate>
	[[nodiscard]] bool anyOf(Predicate&& predicate) const { return findIf(predicate) != end(); }
	class iterator
	{
	protected:
		std::vector<T>::iterator m_iter;
	public:
		iterator(This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		iterator(std::vector<T>::iterator i) : m_iter(i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator--() { --m_iter; return *this; }
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] iterator& operator--(int) { auto copy = *this; --m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
		[[nodiscard]] iterator operator-(const iterator& other) const { return m_iter - other.m_iter; }
		[[nodiscard]] iterator operator+(const iterator& other) const { return m_iter + other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] iterator& operator-=(const iterator& other) { m_iter -= other.m_iter; return *this; }
		[[nodiscard]] iterator operator-(const uint& index) const { return m_iter - index; }
		[[nodiscard]] iterator operator+(const uint& index) const { return m_iter + index; }
		[[nodiscard]] iterator& operator+=(const uint& index) { m_iter += index; return *this; }
		[[nodiscard]] iterator& operator-=(const uint& index) { m_iter -= index; return *this; }
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const { return m_iter <=> other.m_iter; }
		friend class const_iterator;
	};
	class const_iterator
	{
	protected:
		std::vector<T>::const_iterator m_iter;
	public:
		const_iterator(const This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		const_iterator(std::vector<T>::const_iterator i) : m_iter(i) { }
		const_iterator(const const_iterator& i) : m_iter(i.m_iter) { }
		const_iterator(const iterator& i) : m_iter(i.m_iter) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator--() { --m_iter; return *this; }
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		const_iterator& operator--(int) { auto copy = *this; --m_iter; return copy; }
		iterator& operator+=(const const_iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &*m_iter; }
		[[nodiscard]] const_iterator operator-(const iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] const_iterator operator+(const iterator& other) { return m_iter + other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] iterator& operator-=(const iterator& other) { m_iter -= other.m_iter; return *this; }
		[[nodiscard]] const_iterator operator-(const uint& index) { return m_iter - index; }
		[[nodiscard]] const_iterator operator+(const uint& index) { return m_iter + index; }
		[[nodiscard]] iterator& operator+=(const uint& index) { m_iter += index; return *this; }
		[[nodiscard]] iterator& operator-=(const uint& index) { m_iter -= index; return *this; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
	};
	[[nodiscard]] std::string toString() const
	{
		std::string output;
		for(const T& value : m_data)
			output += value.toString() + ", ";
		return output;
	}
	template<typename Source>
	static SmallSet<T> create(const Source& source) { SmallSet<T> output; for(const T& value : source) output.insert(value); return output; }
};
// Define custom serialization / deserialization instead of using intrusive because this type is used in raws and specifiying field name would be annoying.
template<typename T>
inline void to_json(Json& data, const SmallSet<T>& set) { data = set.toJson(); }
template<typename T>
inline void from_json(const Json& data, SmallSet<T>& set) { set.fromJson(data); }

template<typename T>
class SmallSetStable
{
	using This = SmallSetStable<T>;
	std::vector<std::unique_ptr<T>> m_data;
public:
	class iterator;
	class const_iterator;
	SmallSetStable() = default;
	SmallSetStable(std::initializer_list<T> i) : m_data(i) { }
	SmallSetStable(const Json& data) { fromJson(data); }
	void fromJson(const Json& data)
	{
		for(const Json& valueData : data)
			m_data.emplace_back(std::make_unique<T>(valueData));
	}
	[[nodiscard]] Json toJson() const
	{
		Json output = Json::array();
		for(const std::unique_ptr<T>& value : m_data)
			output.push_back(*value);
		return output;
	}
	void insert(const std::unique_ptr<T>& value) { assert(!contains(value)); m_data.push_back(std::move(value)); }
	void maybeInsert(const std::unique_ptr<T>& value) { if(!contains(*value)) m_data.push_back(std::move(value)); }
	void insert(This::iterator begin, This::iterator end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void erase(const std::unique_ptr<T>& value)
	{
		auto found = std::ranges::find(m_data, value);
		assert(found != m_data.end());
		*found = std::move(m_data.back());
		m_data.pop_back();
	}
	void maybeErase(const T& value)
	{
		auto found = std::ranges::find(m_data, value);
		if(found != m_data.end())
		{
			*found = std::move(m_data.back());
			m_data.pop_back();
		}
	}
	void erase(const This::iterator& iter)
	{
		assert(iter != end());
		assert(!m_data.empty());
		uint index = std::distance(m_data.begin(), iter.getIter());
		m_data[index] = std::move(m_data.back());
		m_data.pop_back();
	}
	void erase(const T& value)
	{
		auto iter = find(value);
		erase(iter);
	}
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, [&](const std::unique_ptr<T>& value){ return predicate(*value); }); }
	void clear() { m_data.clear(); }
	template<typename ...Args>
	void emplace(Args&& ...args)
	{
		std::unique_ptr<T> ptr = std::make_unique<T>(std::forward<Args>(args)...);
		assert(std::ranges::find(m_data, ptr) == m_data.end());
		m_data.push_back(std::move(ptr));
	}
	void swap(This& other) { m_data.swap(other.m_data); }
	void popBack() { m_data.pop_back(); }
	template<typename Predicate>
	void sort(Predicate&& predicate) { std::ranges::sort(m_data, predicate); }
	void makeUnique() { std::ranges::sort(m_data); m_data.erase(std::ranges::unique(m_data).begin(), m_data.end()); }
	[[nodiscard]] bool contains(const std::unique_ptr<T>& value) const { return std::ranges::find(m_data, value) != m_data.end(); }
	[[nodiscard]] T& front() { return *m_data.front(); }
	[[nodiscard]] const T& front() const { return *m_data.front(); }
	[[nodiscard]] T& back() { return *m_data.back(); }
	[[nodiscard]] const T& back() const { return *m_data.back(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] This::iterator begin() { return {*this, 0}; }
	[[nodiscard]] This::iterator end() { return {*this, size()}; }
	[[nodiscard]] This::const_iterator begin() const { return This::const_iterator(*this, 0); }
	[[nodiscard]] This::const_iterator end() const { return This::const_iterator(*this, size()); }
	[[nodiscard]] This::iterator find(const T& value) { return std::ranges::find_if(m_data, [&](std::unique_ptr<T>& d){ return *d == value; }); }
	[[nodiscard]] This::const_iterator find(const T& value) const { return const_cast<SmallSetStable&>(*this).find(value); }
	template<typename Predicate>
	[[nodiscard]] This::iterator findIf(Predicate&& predicate) { return std::ranges::find_if(m_data, [&](std::unique_ptr<T>& d){ return predicate(*d); }); }
	template<typename Predicate>
	[[nodiscard]] This::const_iterator findIf(Predicate&& predicate) const { const_cast<SmallSetStable&>(*this).findIf(predicate); }
	template<typename Predicate>
	[[nodiscard]] bool anyOf(Predicate&& predicate) const { return findIf(predicate) != end(); }
	class iterator
	{
	protected:
		std::vector<std::unique_ptr<T>>::iterator m_iter;
	public:
		iterator(This& s, const uint& i) : m_iter(s.m_data.begin() + i) { }
		iterator(std::vector<std::unique_ptr<T>>::iterator i) : m_iter(i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] T& operator*() { return **m_iter; }
		[[nodiscard]] const T& operator*() const { return **m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
		[[nodiscard]] iterator operator-(const iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] iterator operator+(const iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) { return m_iter <=> other.m_iter; }
		[[nodiscard]] const auto& getIter() const { return m_iter; }
		friend class const_iterator;
	};
	class const_iterator
	{
	protected:
		std::vector<std::unique_ptr<T>>::const_iterator m_iter;
	public:
		const_iterator(const This& s, const uint& i) : m_iter(s.m_data.begin() + i) { }
		const_iterator(std::vector<std::unique_ptr<T>>::const_iterator i) : m_iter(i) { }
		const_iterator(const const_iterator& i) : m_iter(i.m_iter) { }
		const_iterator(const iterator& i) : m_iter(i.m_iter) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		iterator& operator+=(const const_iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] const T& operator*() const { return **m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &**m_iter; }
		[[nodiscard]] iterator operator-(const const_iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] iterator operator+(const const_iterator& other) const { return m_iter + other.m_iter; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
		[[nodiscard]] const auto& getIter() const { return m_iter; }
	};
};
// Define custom serialization / deserialization instead of using intrusive because nlohmann doesn't handle unique ptr.
template<typename T>
inline void to_json(Json& data, const SmallSetStable<T>& set) { data = set.toJson(); }
template<typename T>
inline void from_json(const Json& data, SmallSetStable<T>& set) { set.fromJson(data); }