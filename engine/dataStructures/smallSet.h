#pragma once
/*
 * To be used for short seqences of data, not long enough for binary search to make sense.
 * Not pointer stable.
 */
#include "../json.h"
#include <algorithm>
#include <vector>
#include <cassert>

template<typename T>
struct SmallSet
{
	using This = SmallSet<T>;
	std::vector<T> m_data;
	class iterator;
	class const_iterator;
	SmallSet() = default;
	SmallSet(uint capacity);
	SmallSet(std::initializer_list<T> i);
	SmallSet(const This& other) = default;
	SmallSet(This&& other) noexcept = default;
	This& operator=(const This& other) = default;
	This& operator=(This&& other) noexcept = default;
	[[nodiscard]] Json toJson() const;
	void fromJson(const Json& data);
	void insert(const T& value);
	void maybeInsert(const T& value);
	void insert(std::vector<T>::const_iterator begin, const std::vector<T>::const_iterator& end);
	void insert(This::iterator begin, const This::iterator& end);
	void insertNonunique(const T& value);
	void insertFrontNonunique(const T& value);
	void maybeInsertAll(const This& other);
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
	void insertAll(const This& other);
	void insertAllNonunique(const This& other);
	template<typename Iterator>
	void insertAll(Iterator begin, const Iterator& end)
	{
		for(; begin != end; ++begin)
			insert(*begin);
	}
	void erase(const T& value);
	void maybeErase(const T& value);
	void erase(This::iterator iter);
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void eraseAll(This& other);
	template<typename Iterator>
	void maybeEraseAll(Iterator begin, const Iterator& end) { for(; begin != end; ++begin) maybeErase(*begin); }
	void eraseIndex(const uint& index);
	void popBack();
	void clear();
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
	void swap(This& other);
	template<typename Predicate>
	void sort(Predicate&& predicate) { std::ranges::sort(m_data, predicate); }
	void sort();
	void update(const T& oldValue, const T& newValue);
	void updateIfExists(const T& oldValue, const T& newValue);
	void updateIfExistsAndNewValueDoesNot(const T& oldValue, const T& newValue);
	void makeUnique();
	void removeDuplicatesAndValue(const T& value);
	void reserve(uint size);
	void resize(uint size);
	[[nodiscard]] bool operator==(const This& other);
	template<typename Predicate>
	[[nodiscard]] uint countIf(Predicate&& predicate) const  { return std::ranges::count_if(m_data, predicate); }
	[[nodiscard]] const T& operator[](const uint& index) const;
	[[nodiscard]] T& operator[](const uint& index);
	[[nodiscard]] bool contains(const T& value) const;
	template<typename Predicate>
	[[nodiscard]] bool containsAny(Predicate&& predicate) const { return std::ranges::find_if(m_data, predicate) != m_data.end(); }
	[[nodiscard]] uint indexOf(const T& value) const;
	[[nodiscard]] T& front();
	[[nodiscard]] const T& front() const;
	[[nodiscard]] T& back();
	[[nodiscard]] const T& back() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] This::iterator begin();
	[[nodiscard]] This::iterator end();
	[[nodiscard]] This::const_iterator begin() const;
	[[nodiscard]] This::const_iterator end() const;
	[[nodiscard]] std::vector<T>& getVector();
	[[nodiscard]] const std::vector<T>& getVector() const;
	[[nodiscard]] This::iterator find(const T& value);
	[[nodiscard]] This::const_iterator find(const T& value) const;
	[[nodiscard]] bool isUnique() const;
	[[nodiscard]] uint findLastIndex(const T& value) const;
	[[nodiscard]] int maybeFindLastIndex(const T& value) const;
	template<typename Predicate>
	[[nodiscard]] This::iterator findIf(Predicate&& predicate) { return std::ranges::find_if(m_data, predicate); }
	template<typename Predicate>
	[[nodiscard]] This::const_iterator findIf(Predicate&& predicate) const { return std::ranges::find_if(m_data, predicate); }
	template<typename Predicate>
	[[nodiscard]] bool anyOf(Predicate&& predicate) const  { return findIf(predicate) != end(); }
	class iterator
	{
	protected:
		std::vector<T>::iterator m_iter;
	public:
		iterator(This& s, uint i) : m_iter(s.m_data.begin() + i) { }
		iterator(std::vector<T>::iterator i) : m_iter(i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator--() { --m_iter; return *this; }
		[[nodiscard]] iterator operator++(int) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] iterator operator--(int) { auto copy = *this; --m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
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
		const_iterator operator++(int) { auto copy = *this; ++m_iter; return copy; }
		const_iterator operator--(int) { auto copy = *this; --m_iter; return copy; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &*m_iter; }
		[[nodiscard]] const_iterator operator-(const uint& index) { return m_iter - index; }
		[[nodiscard]] const_iterator operator+(const uint& index) { return m_iter + index; }
		[[nodiscard]] const_iterator& operator+=(const uint& index) { m_iter += index; return *this; }
		[[nodiscard]] const_iterator& operator-=(const uint& index) { m_iter -= index; return *this; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
	};
	[[nodiscard]] std::string toString() const;
	static SmallSet<T> create(const auto& source) { SmallSet<T> output; for(const T& value : source) output.insert(value); return output; }
	static SmallSet<T> createNonunique(const auto& source) { SmallSet<T> output; for(const T& value : source) output.insertNonunique(value); return output; }
};
// Define custom serialization / deserialization instead of using intrusive because this type is used in raws and specifiying field name would be annoying.
template<typename T>
inline void to_json(Json& data, const SmallSet<T>& set) { data = set.toJson(); }
template<typename T>
inline void from_json(const Json& data, SmallSet<T>& set) { set.fromJson(data); }

template<typename T>
struct SmallSetStable
{
	using This = SmallSetStable<T>;
	std::vector<std::unique_ptr<T>> m_data;
public:
	class iterator;
	class const_iterator;
	SmallSetStable() = default;
	SmallSetStable(std::initializer_list<T> i);
	SmallSetStable(const Json& data);
	void fromJson(const Json& data);
	[[nodiscard]] Json toJson() const;
	void insert(const std::unique_ptr<T>& value);
	void maybeInsert(const std::unique_ptr<T>& value);
	void insert(This::iterator begin, This::iterator end);
	void erase(const std::unique_ptr<T>& value);
	void maybeErase(const T& value);
	void erase(const This::iterator& iter);
	void erase(const T& value);
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, [&](const std::unique_ptr<T>& value){ return predicate(*value); }); }
	void clear();
	template<typename ...Args>
	void emplace(Args&& ...args)
	{
		std::unique_ptr<T> ptr = std::make_unique<T>(std::forward<Args>(args)...);
		assert(std::ranges::find(m_data, ptr) == m_data.end());
		m_data.push_back(std::move(ptr));
	}
	void swap(This& other);
	void popBack();
	template<typename Predicate>
	void sort(Predicate&& predicate) { std::ranges::sort(m_data, predicate); }
	void makeUnique();
	[[nodiscard]] bool contains(const std::unique_ptr<T>& value) const;
	[[nodiscard]] T& front();
	[[nodiscard]] const T& front() const;
	[[nodiscard]] T& back();
	[[nodiscard]] const T& back() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] This::iterator begin();
	[[nodiscard]] This::iterator end();
	[[nodiscard]] This::const_iterator begin() const;
	[[nodiscard]] This::const_iterator end() const;
	[[nodiscard]] This::iterator find(const T& value);
	[[nodiscard]] This::const_iterator find(const T& value) const;
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