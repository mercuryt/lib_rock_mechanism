/*
 * A cache local map for small data sets made up of a vector of pairs.
 * Keys should be relatively small and values must be move constructable.
 */
#pragma once
#include "json.h"
#include <algorithm>
#include <vector>
#include <cassert>
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
	SmallMap(const SmallMap& other) = default;
	SmallMap(SmallMap&& other) noexcept = default;
	SmallMap& operator=(const SmallMap& other) = default;
	SmallMap& operator=(SmallMap&& other) noexcept = default;
	void insert(const K& key, const V& value)
	{
		assert(!contains(key));
		m_data.emplace_back(key, value);
	}
	void insert(const K& key, V&& value)
	{
		assert(!contains(key));
		m_data.emplace_back(key, std::move(value));
	}
	void maybeInsert(const K& key, const V& value) { if(!contains(key)) insert(key, value); }
	void maybeInsert(const K& key, V&& value) { if(!contains(key)) insert(key, std::forward<V>(value)); }
	void insertNonUnique(const K& key, const V& value)
	{
		m_data.emplace_back(key, value);
	}
	void erase(const K& key)
	{
		erase(std::ranges::find(m_data, key, &Pair::first));
	}
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
	void popBack() { m_data.pop_back(); }
	void clear() { m_data.clear(); }
	template<typename ...Args>
	V& emplace(const K& key, Args&& ...args)
	{
		assert(!contains(key));
		return m_data.emplace_back(key, V{std::forward<Args>(args)...}).second;
	}
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool contains(const K& key) const { return std::ranges::find(m_data, key, &Pair::first) != m_data.end(); }
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
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
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
	[[nodiscard]] const_iterator find(const K& key) const { return std::ranges::find(m_data, key, &Pair::first); }
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
	void clear() { m_data.clear(); }
	template<typename ...Args>
	V& emplace(const K& key, Args&& ...args) { return *m_data.emplace(key, std::make_unique<V>(std::forward<Args>(args)...)).get(); }
	void swap(This& other) { m_data.swap(other.m_data); }
	[[nodiscard]] uint size() const { return m_data.size(); }
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool contains(const K& key) const { return m_data.contains(key); }
	[[nodiscard]] Pair& front() { return m_data.front(); }
	[[nodiscard]] const Pair& front() const { return m_data.front(); }
	[[nodiscard]] Pair& back() { return m_data.back(); }
	[[nodiscard]] const Pair& back() const { return m_data.back(); }
	[[nodiscard]] V& operator[](const K& key) { return *m_data[key].get(); }
	[[nodiscard]] const V& operator[](const K& key) const { return const_cast<This&>(*this)[key]; }
	template<typename Condition>
	[[nodiscard]] iterator findIf(Condition&& condition) { return m_data.findIf([&](const std::pair<K, std::unique_ptr<V>>& pair){ return condition(*pair.second); }); }
	[[nodiscard]] iterator find(const K& key) { return m_data.find(key); }
	[[nodiscard]] const_iterator find(const K& key) const { return findData(key); }
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