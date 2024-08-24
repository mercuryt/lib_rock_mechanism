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
	void insert(T value) { if(!contains(value)) m_data.push_back(value); }
	void insert(std::vector<T>::iterator&& begin, std::vector<T>::iterator&& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void insert(This::iterator begin, This::iterator end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void erase(const T value) { auto found = std::ranges::find(m_data, value); *found = m_data.back(); m_data.pop_back(); }
	template<typename Predicate>
	void erase_if(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void clear() { m_data.clear(); }
	template<typename ...Args>
	void emplace(Args&& ...args)
	{
		// Create first and then assert uniqueness.
		m_data.emplace_back(T(std::forward<Args>(args)...));
		assert(std::ranges::find(m_data.begin(), m_data.end() - 1, m_data.back()) == m_data.end());
	}
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] bool contains(const T value) const { return std::ranges::find(m_data, value) != m_data.end(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] This::iterator begin() { return {*this, 0}; }
	[[nodiscard]] This::iterator end() { return {*this, size()}; }
	[[nodiscard]] This::const_iterator begin() const { return This::const_iterator(*this, 0); }
	[[nodiscard]] This::const_iterator end() const { return This::const_iterator(*this, size()); }
	class iterator
	{
	protected:
		std::vector<T>::iterator m_iter;
	public:
		iterator(This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
	};
	class const_iterator
	{
	protected:
		std::vector<T>::const_iterator m_iter;
	public:
		const_iterator(const This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &*m_iter; }
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
	void insert(K key, V&& value) 
	{ 
		assert(!contains(key));
		m_data.emplace_back(key, std::move(value));
	}
	void erase(const K& key)
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter != m_data.end())
		{
			iter->first = m_data.back().first;
			iter->second = std::move(m_data.back().second);
		}
		m_data.pop_back();
	}
	void clear() { m_data.clear(); }
	template<typename ...Args>
		auto& emplace(const K& key, Args&& ...args)
		{
			assert(!contains(key));
			return m_data.emplace_back(key, V{std::forward<Args>(args)...});
		}
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool contains(const K& key) const { return std::ranges::find(m_data, key, &Pair::first) != m_data.end(); }
	[[nodiscard]] V& operator[](const K& key) 
	{
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		assert(iter != m_data.end());
		return iter->second;
	}
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
	[[nodiscard]] iterator find(const K& key) { return {*this, (uint)std::distance(std::ranges::find(m_data, key, &Pair::first), m_data.begin())}; }
	[[nodiscard]] const_iterator find(const K& key) const { return {*this, (uint)std::distance(std::ranges::find(m_data, key, &Pair::first), m_data.begin())}; }
	[[nodiscard]] iterator begin() { return {*this, 0}; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator begin() const { return {*this, 0}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
	class iterator
	{
		std::vector<Pair>::iterator m_iter;
	public:
		iterator(This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		iterator& operator++() { ++m_iter; return *this; }
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
		const_iterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallMap, keys, values);
};
/*
 * A cache local map for small data sets made up of a vector of pairs.
 * Value addresses are stable.
 * Keys should be relatively small.
 */
template<typename K, typename V>
class SmallMapStable
{
	using This = SmallMapStable<K, V>;
	using Pair = std::pair<K, V>;
	std::vector<Pair> m_data;
public:
	class iterator;
	class const_iterator;
	void insert(K key, V&& value) 
	{ 
		assert(key != K::null()); 
		assert(!contains(key));
		auto iter = std::ranges::find(m_data, K::null());
		m_data.emplace(iter, key, std::move(value));
	}
	void erase(const K& key)
	{
		assert(key != K::null()); 
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		if(iter != m_data.end())
		{
			// Set tombstone
			iter->first = K::null();
			// Destruct value.
			iter->second.~V();
		}
		else
			do 
				m_data.pop_back();
			while(m_data.back() == K::null());
	}
	void clear() { m_data.clear(); }
	template<typename ...Args>
	auto& emplace(const K& key, Args&& ...args)
	{
		assert(key != K::null()); 
		assert(!contains(key));
		auto iter = std::ranges::find(m_data, K::null());
		if(iter == m_data.end())
			return m_data.emplace_back(key, V{std::forward<Args>(args)...});
		else
		{
			iter->first = key;
			// Construct in place.
			new(&iter->second) V(std::forward<Args>(args)...);
		}
	}
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool contains(const K& key) const { assert(key != K::null()); return std::ranges::find(m_data, key, &Pair::first) != m_data.end(); }
	[[nodiscard]] V& operator[](const K& key) 
	{
		assert(key != K::null()); 
		auto iter = std::ranges::find(m_data, key, &Pair::first);
		assert(iter != m_data.end());
		return iter->second;
	}
	[[nodiscard]] const V& operator[](const K& key) const { assert(key != K::null()); return const_cast<This&>(*this)[key]; }
	[[nodiscard]] iterator find(const K& key) { assert(key != K::null()); return {*this, (uint)std::distance(std::ranges::find(m_data, key, &Pair::first), m_data.begin())}; }
	[[nodiscard]] const_iterator find(const K& key) const { assert(key != K::null()); return {*this, (uint)std::distance(std::ranges::find(m_data, key, &Pair::first), m_data.begin())}; }
	[[nodiscard]] iterator firstNonNull() { return std::ranges::find_if_not(m_data, K::null(), &Pair::first); }
	[[nodiscard]] const_iterator firstNonNullConst() const { return const_cast<This&>(*this).firstNonNull(); }
	[[nodiscard]] iterator begin() { return {*this, firstNonNull()}; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator begin() const { return {*this, firstNonNullConst()}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
	class iterator
	{
		std::vector<Pair>::iterator m_iter;
	public:
		iterator(This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		iterator& operator++() { do{++m_iter;}while(*m_iter == K::null()); return *this; }
		[[nodiscard]] iterator& operator++(int) { auto copy = *this; do{++m_iter;}while(*m_iter == K::null()); return copy; }
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
		const_iterator& operator++() { do{++m_iter;}while(*m_iter == K::null()); return *this; }
		[[nodiscard]] const_iterator& operator++(int) { auto copy = *this; do{++m_iter;}while(*m_iter == K::null()); return copy; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SmallMapStable, keys, values);
};
