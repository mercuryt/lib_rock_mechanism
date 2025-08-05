/*
 * A cache local map for small data sets made up of a vector of pairs.
 * Keys should be relatively small and values must be move constructable.
 */
#pragma once
#include "json.h"
#include "concepts.h"
#include <algorithm>
#include <vector>
#include <cassert>
template<typename K, MoveConstructible V>
class SmallMap
{
	using This = SmallMap<K, V>;
	using Pair = std::pair<K, V>;
	using Data =std::vector<Pair>;
	Data m_data;
public:
	class iterator;
	class const_iterator;
	SmallMap() = default;
	SmallMap(const std::initializer_list<Pair>& i);
	SmallMap(const SmallMap& other) = default;
	SmallMap(SmallMap&& other) noexcept = default;
	SmallMap& operator=(const SmallMap& other) = default;
	SmallMap& operator=(SmallMap&& other) noexcept = default;
	void insert(const K& key, const V& value);
	void insert(const K& key, V&& value);
	void maybeInsert(const K& key, const V& value);
	void maybeInsert(const K& key, V&& value);
	void insertNonUnique(const K& key, const V& value);
	void erase(const K& key);
	void erase(iterator& iter);
	void erase(iterator&& iter);
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void maybeErase(const K& key);
	bool maybeEraseNotify(const K& key);
	void popBack();
	void clear();
	template<typename ...Args>
	V& emplace(const K& key, Args&& ...args)
	{
		assert(!contains(key));
		return m_data.emplace_back(key, V{std::forward<Args>(args)...}).second;
	}
	template<typename Condition>
	void sort(Condition&& condition)
	{
		auto pairCondition = [&](const Pair& a, const Pair& b) { return condition(a.first, b.first); };
		std::ranges::sort(m_data, pairCondition);
	}
	void swap(This& other);
	void updateKey(const K& oldKey, const K& newKey);
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool contains(const K& key) const;
	[[nodiscard]] Pair& front();
	[[nodiscard]] const Pair& front() const;
	[[nodiscard]] Pair& back();
	[[nodiscard]] const Pair& back() const;
	[[nodiscard]] V& operator[](const K& key);
	[[nodiscard]] const V& operator[](const K& key) const;
	[[nodiscard]] V& getOrInsert(const K& key, V&& value);
	[[nodiscard]] V& getOrCreate(const K& key);
	template<typename Condition>
	[[nodiscard]] iterator findIf(Condition&& condition) { return std::ranges::find_if(m_data, condition); }
	[[nodiscard]] iterator find(const K& key);
	[[nodiscard]] const_iterator find(const K& key) const;
	[[nodiscard]] iterator begin();
	[[nodiscard]] iterator end();
	[[nodiscard]] const_iterator begin() const;
	[[nodiscard]] const_iterator end() const;
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
		[[nodiscard]] iterator operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] Pair& operator*() { return *m_iter; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() { return m_iter->first; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] V& second() { return m_iter->second; }
		[[nodiscard]] const V& second() const { return m_iter->second; }
		[[nodiscard]] Pair* operator->() { return &*m_iter; }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
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
		[[nodiscard]] const_iterator operator++(int) { auto copy = *this; ++m_iter; return copy; }
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
	Data::iterator findData(const K& key);
	Data::const_iterator findData(const K& key) const;
public:
	class iterator;
	class const_iterator;
	SmallMapStable() = default;
	SmallMapStable(std::initializer_list<Pair>&& i);
	SmallMapStable(const Json& data);
	void fromJson(const Json& data);
	[[nodiscard]] Json toJson() const;
	V& insert(const K& key, std::unique_ptr<V>&& value);
	void erase(const K& key);
	void clear();
	template<typename ...Args>
	V& emplace(const K& key, Args&& ...args) { return *m_data.emplace(key, std::make_unique<V>(std::forward<Args>(args)...)).get(); }
	void swap(This& other);
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool contains(const K& key) const;
	[[nodiscard]] Pair& front();
	[[nodiscard]] const Pair& front() const;
	[[nodiscard]] Pair& back();
	[[nodiscard]] const Pair& back() const;
	[[nodiscard]] auto operator[](const K& key) -> V&;
	[[nodiscard]] auto operator[](const K& key) const -> const V&;
	template<typename Condition>
	[[nodiscard]] iterator findIf(Condition&& condition) { return m_data.findIf([&](const Pair& pair){ return condition(*pair.second); }); }
	[[nodiscard]] iterator find(const K& key);
	[[nodiscard]] const_iterator find(const K& key) const;
	[[nodiscard]] iterator begin();
	[[nodiscard]] iterator end();
	[[nodiscard]] const_iterator begin() const;
	[[nodiscard]] const_iterator end() const;
	[[nodiscard]] V& getOrCreate(const K& key);
	class iterator
	{
		Data::iterator m_iter;
	public:
		iterator(This& map, uint i) : m_iter(map.m_data.begin() + i) { }
		iterator(Data::iterator& iter) : m_iter(iter) { }
		iterator(Data::iterator iter) : m_iter(iter) { }
		iterator& operator++() { ++m_iter; return *this; }
		[[nodiscard]] iterator operator++(int) { iterator copy = *this; ++m_iter; return copy; }
		[[nodiscard]] Pair& operator*() { return *m_iter; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() { return m_iter->first; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] V& second() { return *m_iter->second; }
		[[nodiscard]] const V& second() const { return *m_iter->second.get(); }
		[[nodiscard]] Pair* operator->() { return &*m_iter; }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
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
		[[nodiscard]] const_iterator operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] const Pair& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const K& first() const { return m_iter->first; }
		[[nodiscard]] const V& second() const { return *m_iter->second.get(); }
		[[nodiscard]] const Pair* operator->() const { return &*m_iter; }
		[[nodiscard]] const_iterator operator+(uint steps) { return m_iter + steps; }
		[[nodiscard]] const_iterator operator-(uint steps) { return m_iter - steps; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
	};
};
// Define custom serialization / deserialization instead of using intrusive because nlohmann doesn't handle unique ptr.
template<typename K, typename V>
inline void to_json(Json& data, const SmallMapStable<K, V>& set) { data = set.toJson(); }
template<typename K, typename V>
inline void from_json(const Json& data, SmallMapStable<K, V>& set) { set.fromJson(data); }