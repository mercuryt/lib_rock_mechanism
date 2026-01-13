#pragma once
#include <vector>
#include <algorithm>
// Sorted on demand, uses binary search.
template<typename T>
class MediumSet
{
	std::vector<T> data;
public:
	class iterator;
	class const_iterator;
	void add(const T& value) { data.push_back(value); std::ranges::sort(data); }
	void maybeAdd(const T& value) { if(!contains(value)) add(value); }
	void remove(const iterator& iter) { data.erase(iter); }
	void remove(const T& value) { auto iter = find(value); assert(iter != data.end()); remove(iter); }
	void maybeRemove(const T& value) { auto iter = find(value); if(iter != data.end()) remove(iter); }
	template<typename ...Args>
	void emplace(Args&& ...args) {
		T value{std::forward(args)...};
		data.push_back(std::move(value));
		std::ranges::sort(data);
	}
	[[nodiscard]] bool contains(const T& value) const { return std::binary_search(data.begin(), data.end(), value); }
	[[nodiscard]] int size() const { return data.size(); }
	[[nodiscard]] iterator find(const T& value)
	{
		auto iter = std::lower_bound(data.begin(), data.end(), value);
		if(*iter == value)
			return {*this, (int)std::distance(data.begin(), iter)};
		return end();
	}
	[[nodiscard]] const_iterator find(const T& value) const { return const_cast<MediumSet<T>*>(this)->find(value); }
	[[nodiscard]] iterator begin() { return {*this, 0}; }
	[[nodiscard]] const_iterator begin() const { return {*this, 0}; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
	class iterator
	{
		std::vector<T>::iterator m_iter;
	public:
		iterator(MediumSet<T>& set, int i) : m_iter(set.data.begin() + i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
	};
	class const_iterator
	{
		std::vector<T>::const_iterator m_iter;
	public:
		const_iterator(const MediumSet<T>& set, int i) : m_iter(set.data.begin() + i) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const T& operator*() const { *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
	};
};
template<typename K, typename V>
class MediumMap
{
	using This = MediumMap<K, V>;
	using Pair = std::pair<K, V>;
	std::vector<Pair> data;
public:
	class iterator;
	class const_iterator;
	void insert(const K& key, const V& value) { assert(!contains(key)); data.emplace_back(key, value); std::ranges::sort(data); }
	void insertNonUnique(const K& key, const V& value) { data.emplace_back(key, value); std::ranges::sort(data); }
	void maybeInsert(const K& key, const V& value) { if(!contains(key)) { data.emplace_back(key, value); std::ranges::sort(data); } }
	void remove(const K& key) { assert(contains(key)); auto iter = find(key); *iter = data.back(); data.pop_back(); std::ranges::sort(data); }
	template<typename ...Args>
	void emplace(const K& key, Args&& ...args) {
		V value{std::forward(args)...};
		Pair pair{key, value};
		//TODO: instead of push_back / sort use lower_bound and insert.
		data.push_back(std::move(pair));
		std::ranges::sort(data);
	}
	void pop_back() { data.pop_back(); }
	void clear() { data.clear(); }
	[[nodiscard]] bool contains(const K& key) const { return std::ranges::binary_search(data, key, {}, &Pair::first); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] int size() const { return data.size(); }
	[[nodiscard]] Pair& back() { return data.back(); }
	[[nodiscard]] iterator find(const K& key)
	{
		auto iter = std::ranges::lower_bound(data, key, &Pair::first);
		if(iter->first == key)
			return {*this, iter - data.begin()};
		return end();
	}
	[[nodiscard]] const_iterator find(const K& key) const { return const_cast<This*>(this)->find(key); }
	[[nodiscard]] V& operator[](const K& key) { auto iter = find(key); assert(key != data.end()); return iter->second; }
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
	[[nodiscard]] iterator begin() { return {*this, 0}; }
	[[nodiscard]] const_iterator begin() const { return {*this, 0}; }
	[[nodiscard]] iterator end() { return {*this, size()}; }
	[[nodiscard]] const_iterator end() const { return {*this, size()}; }
	class iterator
	{
		std::vector<Pair>::iterator m_iter;
	public:
		iterator(This& set, int i) : m_iter(set.data.begin() + i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] Pair& operator*() { *m_iter; }
		[[nodiscard]] const Pair& operator*() const { *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
	};
	class const_iterator
	{
		std::vector<Pair>::const_iterator m_iter;
	public:
		const_iterator(const This& set, int i) : m_iter(set.data.begin() + i) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const Pair& operator*() const { *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
	};
};
