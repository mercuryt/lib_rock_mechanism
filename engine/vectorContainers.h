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
	SmallSet(std::initializer_list<T> i) : m_data(i) { }
	void insert(const T& value) { if(!contains(value)) m_data.push_back(value); }
	void insert(std::vector<T>::const_iterator&& begin, std::vector<T>::const_iterator&& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void insert(This::const_iterator begin, This::const_iterator end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void erase(const T& value)
	{
		auto found = std::ranges::find(m_data, value);
		if(found != m_data.end())
		{
			*found = m_data.back();
			m_data.pop_back();
		}
	}
	template<typename Predicate>
	void erase_if(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void clear() { m_data.clear(); }
	template<typename ...Args>
	void emplace(Args&& ...args)
	{
		// Create first and then assert uniqueness.
		const auto& created = m_data.emplace_back(T(std::forward<Args>(args)...));
		assert(std::ranges::count(m_data, created) == 1);
	}
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] bool contains(const T& value) const { return std::ranges::find(m_data, value) != m_data.end(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] This::iterator begin() { return {*this, 0}; }
	[[nodiscard]] This::iterator end() { return {*this, size()}; }
	[[nodiscard]] This::const_iterator begin() const { return This::const_iterator(*this, 0); }
	[[nodiscard]] This::const_iterator end() const { return This::const_iterator(*this, size()); }
	[[nodiscard]] std::vector<T>& getVector() { return m_data; }
	class iterator
	{
	protected:
		std::vector<T>::iterator m_iter;
	public:
		iterator(This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		iterator(std::vector<T>::iterator i) : m_iter(i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
		// Used by Open MP.
		[[nodiscard]] iterator operator-(const iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
		friend class const_iterator;
	};
	class const_iterator
	{
	protected:
		std::vector<T>::const_iterator m_iter;
	public:
		const_iterator(const This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		const_iterator(std::vector<T>::const_iterator i) : m_iter(i) { }
		const_iterator(const iterator& i) : m_iter(i.m_iter) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &*m_iter; }
		[[nodiscard]] iterator operator-(const iterator& other) { return m_iter - other.m_iter; }
		[[nodiscard]] iterator& operator+=(const iterator& other) { m_iter += other.m_iter; return *this; }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallSet, m_data);
};

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
	void insert(K key, V& value)
	{
		assert(!contains(key));
		m_data.emplace_back(key, std::move(value));
	}
	void insert(K key, V&& value) { insert(key, value); }
	void erase(const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		assert(iter != m_data.end());
		iter->first = m_data.back().first;
		iter->second = std::move(m_data.back().second);
		m_data.pop_back();
	}
	void erase(const K&& key) { erase(key); }
	void maybeErase(const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter == m_data.end())
			return;
		iter->first = m_data.back().first;
		iter->second = std::move(m_data.back().second);
		m_data.pop_back();
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
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] Pair& operator*() { return *m_iter; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] V& second() { return m_iter->second; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] Pair* operator->() { return &*m_iter; }
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
		[[nodiscard]] const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
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
	[[nodiscard]] V& operator[](const K& key) { return *m_data[key].get(); }
	[[nodiscard]] V& operator[](const K&& key) { return *(*this)[key].get(); }
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
	[[nodiscard]] const V& operator[](const K&& key) const { return const_cast<This&>(*this)[key]; }
	[[nodiscard]] iterator find(const K& key) { return m_data.find(key); }
	[[nodiscard]] iterator find(const K&& key) { return find(key); }
	[[nodiscard]] const_iterator find(const K& key) const { return findData(key); }
	[[nodiscard]] const_iterator find(const K&& key) const { return find(key); }
	[[nodiscard]] iterator begin() { return {*this, 0} ; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator begin() const { return {*this, 0}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
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
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] V& second() { return *m_iter->second.get(); }
		[[nodiscard]] const V& second() const { return *m_iter->second.get(); }
		[[nodiscard]] Pair* operator->() { return &*m_iter; }
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
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return *m_iter->second.get(); }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallMapStable, m_data);
};
