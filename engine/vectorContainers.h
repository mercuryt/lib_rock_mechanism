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
	SmallSet(const This& other) { m_data = other.m_data; }
	SmallSet(This&& other) { m_data = std::move(other.m_data); }
	This& operator=(const This& other) { m_data = other.m_data; return *this; }
	This& operator=(This&& other) { m_data = std::move(other.m_data); return *this; }
	[[nodiscard]] Json toJson() const { return m_data; }
	void fromJson(const Json& data)
	{
		for(const Json& valueData : data)
			m_data.emplace_back(valueData);
	}
	void insert(const T& value) { assert(!contains(value)); m_data.push_back(value); }
	void maybeInsert(const T& value) { if(!contains(value)) m_data.push_back(value); }
	void insert(std::vector<T>::const_iterator&& begin, std::vector<T>::const_iterator&& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void insert(This::iterator begin, This::iterator end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void insertNonunique(const T& value) { m_data.push_back(value); }
	void maybeInsertAll(const This& other)
	{
		maybeInsertAll(other.begin(), other.end());
	}
	template<typename Iterator>
	void maybeInsertAll(Iterator begin, Iterator end)
	{
		for(; begin != end; ++begin)
			insertNonunique(*begin);
		makeUnique();
	}
	void insertAll(const This& other)
	{
		insertAll(other.begin(), other.end());
	}
	template<typename Iterator>
	void insertAll(Iterator begin, Iterator end)
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
	void maybeEraseAll(Iterator begin, Iterator end)
	{
		for(; begin != end; ++begin)
			maybeErase(*begin);
	}
	void eraseIndex(const uint& index)
	{
		erase(m_data.begin() + index);
	}
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
	void popBack() { m_data.pop_back(); }
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
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
		[[nodiscard]] iterator operator-(const iterator& other) const { return m_iter - other.m_iter; }
		[[nodiscard]] iterator operator+(const iterator& other) const { return m_iter + other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] iterator& operator-=(const iterator& other) { m_iter -= other.m_iter; return *this; }
		[[nodiscard]] iterator operator-(const uint& index) const { return {m_iter - index}; }
		[[nodiscard]] iterator operator+(const uint& index) const { return {m_iter + index}; }
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
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		iterator& operator+=(const const_iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &*m_iter; }
		[[nodiscard]] iterator operator-(const iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] iterator operator+(const iterator& other) { return m_iter + other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
		[[nodiscard]] iterator& operator-=(const iterator& other) { m_iter -= other.m_iter; return *this; }
		[[nodiscard]] iterator operator-(const uint& index) { return {m_iter - index}; }
		[[nodiscard]] iterator operator+(const uint& index) { return {m_iter + index}; }
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
	void insert(const std::unique_ptr<T> value) { assert(!contains(value)); m_data.push_back(std::move(value)); }
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
	void erase(This::iterator iter)
	{
		assert(iter != end());
		assert(!m_data.empty());
		uint index = std::distance(m_data.begin(), iter.getIter());
		m_data[index] = std::move(m_data.back());
		m_data.pop_back();
	}
	void erase(T& value)
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
		iterator(This& s, uint i) : m_iter(s.m_data.begin() + i) { }
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
		const_iterator(const This& s, uint i) : m_iter(s.m_data.begin() + i) { }
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

/*
 * A cache local map for small data sets made up of a vector of pairs.
 * Keys should be relatively small and values must be move constructable.
 */
template<typename T>
concept MoveConstructible = std::is_move_constructible_v<T>;
template<typename K, MoveConstructible V>
class SmallMap
{
	using This = SmallMap<K, V>;
	using Pair = std::pair<K, V>;
	std::vector<Pair> m_data;
public:
	class iterator;
	class const_iterator;
	SmallMap() = default;
	SmallMap(const std::initializer_list<std::pair<K, V>>& i) : m_data(i) { }
	SmallMap(std::initializer_list<std::pair<K, V>>&& i) : m_data(i) { }
	SmallMap(const SmallMap& other) : m_data(other.m_data) { }
	SmallMap(SmallMap&& other) noexcept : m_data(std::move(other.m_data)) { }
	SmallMap& operator=(const SmallMap& other) { m_data = other.m_data; return *this; }
	SmallMap& operator=(SmallMap&& other) noexcept { m_data = std::move(other.m_data); return *this; }
	void insert(K key, V& value)
	{
		assert(!contains(key));
		m_data.emplace_back(key, std::move(value));
	}
	void insert(K key, V&& value) { insert(key, value); }
	void maybeInsert(K key, V& value) { if(!contains(key)) insert(key, value); }
	void maybeInsert(K key, V&& value) { maybeInsert(key, value); }
	void insertNonUnique(K key, V& value)
	{
		m_data.emplace_back(key, std::move(value));
	}
	void erase(const K& key)
	{
		erase(std::ranges::find(m_data, key, &Pair::first));
	}
	void erase(const K&& key) { erase(key); }
	void erase(iterator& iter)
	{
		assert(iter != m_data.end());
		if(iter != m_data.end() - 1)
		{
			iter->first = m_data.back().first;
			iter->second = std::move(m_data.back().second);
		}
		m_data.pop_back();
	}
	void erase(iterator&& iter) { erase(iter); }
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void maybeErase(const K& key)
	{
		iterator iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter != m_data.end())
			erase(iter);
	}
	bool maybeEraseNotify(const K& key)
	{
		iterator iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter != m_data.end())
		{
			erase(iter);
			return true;
		}
		return false;
	}
	void clear() { m_data.clear(); }
	template<typename ...Args>
	V& emplace(const K& key, Args&& ...args)
	{
		assert(!contains(key));
		return m_data.emplace_back(key, V{std::forward<Args>(args)...}).second;
	}
	template<typename ...Args>
	V& emplace(const K&& key, Args&& ...args) { return emplace(key, std::forward<Args>(args)...);  }
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool contains(const K& key) const { return std::ranges::find(m_data, key, &Pair::first) != m_data.end(); }
	[[nodiscard]] bool contains(const K&& key) const { return contains(key); }
	[[nodiscard]] Pair& front() { return m_data.front(); }
	[[nodiscard]] const Pair& front() const { return m_data.front(); }
	[[nodiscard]] Pair& back() { return m_data.back(); }
	[[nodiscard]] const Pair& back() const { return m_data.back(); }
	[[nodiscard]] V& operator[](const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		assert(iter != m_data.end());
		return iter->second;
	}
	[[nodiscard]] V& operator[](const K&& key) { return (*this)[key]; }
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
	[[nodiscard]] const V& operator[](const K&& key) const { return (*this)[key]; }
	[[nodiscard]] V& createEmpty(const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		assert(iter == m_data.end());
		return m_data.emplace_back(key, V{}).second;
	}
	[[nodiscard]] V& getOrInsert(const K& key, V&& value)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter == m_data.end())
			return m_data.emplace_back(key, value).second;
		return iter->second;
	}
	[[nodiscard]] V& getOrCreate(const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter == m_data.end())
			return m_data.emplace_back(key, V{}).second;
		return iter->second;
	}
	template<typename Condition>
	[[nodiscard]] iterator findIf(Condition&& condition) { return std::ranges::find_if(m_data, condition); }
	[[nodiscard]] iterator find(const K& key) { return std::ranges::find(m_data, key, &Pair::first); }
	[[nodiscard]] iterator find(const K&& key) { return find(key); }
	[[nodiscard]] const_iterator find(const K& key) const { return std::ranges::find(m_data, key, &Pair::first); }
	[[nodiscard]] const_iterator find(const K&& key) const { return find(key); }
	[[nodiscard]] iterator begin() { return {*this, 0}; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator begin() const { return {*this, 0}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
	class iterator
	{
		std::vector<Pair>::iterator m_iter;
	public:
		iterator(This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		iterator(std::vector<Pair>::iterator& iter) : m_iter(iter) { }
		iterator(std::vector<Pair>::iterator iter) : m_iter(iter) { }
		iterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] iterator operator+(uint steps) { return m_iter + steps; }
		[[nodiscard]] iterator operator-(uint steps) { return m_iter - steps; }
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] Pair& operator*() { return *m_iter; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] V& second() { return m_iter->second; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] Pair* operator->() { return &*m_iter; }
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const { return m_iter <=> other.m_iter; }
	};
	class const_iterator
	{
		std::vector<Pair>::const_iterator m_iter;
	public:
		const_iterator(const This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		const_iterator(std::vector<Pair>::const_iterator& iter) : m_iter(iter) { }
		const_iterator(std::vector<Pair>::const_iterator iter) : m_iter(iter) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] const_iterator operator+(uint steps) { return m_iter + steps; }
		[[nodiscard]] const_iterator operator-(uint steps) { return m_iter - steps; }
		[[nodiscard]] const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallMap, m_data);
};
template<typename K, typename V>
class SmallMapStable
{
	using This = SmallMapStable<K, V>;
	using Pair = std::pair<K, std::unique_ptr<V>>;
	SmallMap<K, std::unique_ptr<V>> m_data;
	using Data = decltype(m_data);
	Data::iterator findData(const K& key) { return std::ranges::find_if(m_data, [&](const auto& pair) { return key == pair.first; }); }
	Data::const_iterator findData(const K& key) const { return findData(key); }
public:
	class iterator;
	class const_iterator;
	SmallMapStable() = default;
	SmallMapStable(const std::initializer_list<std::pair<K, V>>& i) : m_data(i) { }
	SmallMapStable(const Json& data) { fromJson(data); }
	void fromJson(const Json& data)
	{
		for(const Json& pair : data)
			m_data.emplace(pair[0], std::unique_ptr<V>(pair[1]));
	}
	[[nodiscard]] Json toJson() const
	{
		Json output = Json::array();
		for(const auto& [key, value] : m_data)
			output.push_back({key, *value});
		return output;
	}
	auto insert(const K& key, std::unique_ptr<V>&& value) -> V& { m_data.insert(key, std::move(value)); return *m_data[key]; }
	void erase(const K& key) { m_data.erase(key); }
	void erase(const K&& key) { erase(key); }
	void clear() { m_data.clear(); }
	template<typename ...Args>
	V& emplace(const K& key, Args&& ...args) { return *m_data.emplace(key, std::make_unique<V>(std::forward<Args>(args)...)).get(); }
	template<typename ...Args>
	V& emplace(const K&& key, const Args&& ...args) { return emplace(key, std::forward<Args>(args)...);  }
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool contains(const K& key) const { return m_data.contains(key); }
	[[nodiscard]] bool contains(const K&& key) const { return contains(key); }
	[[nodiscard]] Pair& front() { return m_data.front(); }
	[[nodiscard]] const Pair& front() const { return m_data.front(); }
	[[nodiscard]] Pair& back() { return m_data.back(); }
	[[nodiscard]] const Pair& back() const { return m_data.back(); }
	[[nodiscard]] V& operator[](const K& key) { return *m_data[key].get(); }
	[[nodiscard]] V& operator[](const K&& key) { return *(*this)[key].get(); }
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
	[[nodiscard]] const V& operator[](const K&& key) const { return const_cast<This&>(*this)[key]; }
	template<typename Condition>
	[[nodiscard]] iterator findIf(Condition&& condition) { return m_data.findIf([&](const std::pair<K, std::unique_ptr<V>>& pair){ return condition(*pair.second); }); }
	[[nodiscard]] iterator find(const K& key) { return m_data.find(key); }
	[[nodiscard]] iterator find(const K&& key) { return find(key); }
	[[nodiscard]] const_iterator find(const K& key) const { return findData(key); }
	[[nodiscard]] const_iterator find(const K&& key) const { return find(key); }
	[[nodiscard]] iterator begin() { return {*this, 0} ; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator begin() const { return {*this, 0}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
	[[nodiscard]] V& getOrCreate(const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter == m_data.end())
			return *(m_data.emplace_back(key, std::make_unique<V>()).second);
		return *(iter->second);
	}
	class iterator
	{
		Data::iterator m_iter;
	public:
		iterator(This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		iterator(Data::iterator& iter) : m_iter(iter) { }
		iterator(Data::iterator iter) : m_iter(iter) { }
		iterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] Pair& operator*() { return *m_iter; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] V& second() { return *m_iter->second.get(); }
		[[nodiscard]] const V& second() const { return *m_iter->second.get(); }
		[[nodiscard]] Pair* operator->() { return &*m_iter; }
		[[nodiscard]] iterator operator+(uint steps) { return m_iter + steps; }
		[[nodiscard]] iterator operator-(uint steps) { return m_iter - steps; }
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const { return m_iter <=> other.m_iter; }
	};
	class const_iterator
	{
		Data::const_iterator m_iter;
	public:
		const_iterator(const This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		const_iterator(Data::const_iterator& iter) : m_iter(iter) { }
		const_iterator(Data::const_iterator iter) : m_iter(iter) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return *m_iter->second.get(); }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
		[[nodiscard]] iterator operator+(uint steps) { return m_iter + steps; }
		[[nodiscard]] iterator operator-(uint steps) { return m_iter - steps; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
	};
};
// Define custom serialization / deserialization instead of using intrusive because nlohmann doesn't handle unique ptr.
template<typename K, typename V>
inline void to_json(Json& data, const SmallMapStable<K, V>& set) { data = set.toJson(); }
template<typename K, typename V>
inline void from_json(const Json& data, SmallMapStable<K, V>& set) { set.fromJson(data); }