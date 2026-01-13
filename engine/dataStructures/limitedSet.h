#pragma once
/*
 * To be used for short seqences of data, not long enough for binary search to make sense.
 * Not pointer stable.
 */
#include "../json.h"
#include "../concepts.h"
#include <algorithm>
#include <array>
#include <cassert>

template<typename T, int32_t capacity>
class LimitedSet
{
	using This = LimitedSet<T, capacity>;
	std::array<T, capacity> m_data;
	int32_t m_size = 0;
public:
	class iterator;
	class const_iterator;
	LimitedSet() = default;
	LimitedSet(const std::initializer_list<T>& i);
	LimitedSet(const This& other) = default;
	LimitedSet(This&& other) noexcept = default;
	This& operator=(const This& other) = default;
	This& operator=(This&& other) noexcept = default;
	[[nodiscard]] Json toJson() const;
	void fromJson(const Json& data);
	void insert(const T& value);
	void maybeInsert(const T& value);
	void insert(std::array<T, capacity>::const_iterator begin, const std::array<T, capacity>::const_iterator& end);
	void insert(iterator begin, const iterator& end);
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
	void maybeInsertAll(const auto& source) { maybeInsertAll(source.begin(), source.end()); }
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
	void erase(iterator iter);
	template<typename Predicate>
	void eraseIf(Predicate&& predicate) { std::erase_if(m_data, predicate); }
	void eraseAll(This& other);
	template<typename Iterator>
	void maybeEraseAll(Iterator begin, const Iterator& end) { for(; begin != end; ++begin) maybeErase(*begin); }
	void eraseIndex(const int32_t& index);
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
	[[nodiscard]] bool operator==(const This& other);
	template<typename Predicate>
	[[nodiscard]] int32_t countIf(Predicate&& predicate) const  { return std::ranges::count_if(m_data, predicate); }
	[[nodiscard]] const T& operator[](const int32_t& index) const;
	[[nodiscard]] T& operator[](const int32_t& index);
	[[nodiscard]] bool contains(const T& value) const;
	template<typename Predicate>
	[[nodiscard]] bool containsAny(Predicate&& predicate) const { return std::ranges::find_if(m_data, predicate) != m_data.end(); }
	[[nodiscard]] int32_t indexOf(const T& value) const;
	[[nodiscard]] T& front();
	[[nodiscard]] const T& front() const;
	[[nodiscard]] T& back();
	[[nodiscard]] const T& back() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] int32_t size() const;
	[[nodiscard]] This::iterator begin();
	[[nodiscard]] This::iterator end();
	[[nodiscard]] This::const_iterator begin() const;
	[[nodiscard]] This::const_iterator end() const;
	[[nodiscard]] auto& getArray() { return m_data; }
	[[nodiscard]] const auto& getArray() const { return m_data; }
	[[nodiscard]] This::iterator find(const T& value);
	[[nodiscard]] This::const_iterator find(const T& value) const;
	[[nodiscard]] bool isUnique() const;
	[[nodiscard]] int32_t findLastIndex(const T& value) const;
	template<typename Predicate>
	[[nodiscard]] This::iterator findIf(Predicate&& predicate) { return std::ranges::find_if(m_data, predicate); }
	template<typename Predicate>
	[[nodiscard]] This::const_iterator findIf(Predicate&& predicate) const { return std::ranges::find_if(m_data, predicate); }
	template<typename Predicate>
	[[nodiscard]] bool anyOf(Predicate&& predicate) const  { return findIf(predicate) != end(); }
	class iterator
	{
	protected:
		std::array<T, capacity>::iterator m_iter;
	public:
		iterator(This& s, int32_t i) : m_iter(s.m_data.begin() + i) { }
		iterator(std::array<T, capacity>::iterator i) : m_iter(i) { }
		iterator& operator++() { ++m_iter; return *this; }
		iterator& operator--() { --m_iter; return *this; }
		[[nodiscard]] iterator operator++(int32_t) { auto copy = *this; ++m_iter; return copy; }
		[[nodiscard]] iterator operator--(int32_t) { auto copy = *this; --m_iter; return copy; }
		[[nodiscard]] T& operator*() { return *m_iter; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] T* operator->() { return &*m_iter; }
		[[nodiscard]] iterator operator-(const int32_t& index) const { return m_iter - index; }
		[[nodiscard]] iterator operator+(const int32_t& index) const { return m_iter + index; }
		[[nodiscard]] iterator& operator+=(const int32_t& index) { m_iter += index; return *this; }
		[[nodiscard]] iterator& operator-=(const int32_t& index) { m_iter -= index; return *this; }
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const { return m_iter <=> other.m_iter; }
		friend class const_iterator;
	};
	class const_iterator
	{
	protected:
		std::array<T, capacity>::const_iterator m_iter;
	public:
		const_iterator(const This& s, int32_t i) : m_iter(s.m_data.begin() + i) { }
		const_iterator(std::array<T, capacity>::const_iterator i) : m_iter(i) { }
		const_iterator(const const_iterator& i) : m_iter(i.m_iter) { }
		const_iterator(const iterator& i) : m_iter(i.m_iter) { }
		const_iterator& operator++() { ++m_iter; return *this; }
		const_iterator& operator--() { --m_iter; return *this; }
		const_iterator operator++(int32_t) { auto copy = *this; ++m_iter; return copy; }
		const_iterator operator--(int32_t) { auto copy = *this; --m_iter; return copy; }
		[[nodiscard]] const T& operator*() const { return *m_iter; }
		[[nodiscard]] bool operator==(const const_iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator==(const iterator& other) const { return m_iter == other.m_iter; }
		[[nodiscard]] bool operator!=(const const_iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return m_iter != other.m_iter; }
		[[nodiscard]] const T* operator->() const { return &*m_iter; }
		[[nodiscard]] const_iterator operator-(const int32_t& index) { return m_iter - index; }
		[[nodiscard]] const_iterator operator+(const int32_t& index) { return m_iter + index; }
		[[nodiscard]] const_iterator& operator+=(const int32_t& index) { m_iter += index; return *this; }
		[[nodiscard]] const_iterator& operator-=(const int32_t& index) { m_iter -= index; return *this; }
		[[nodiscard]] std::strong_ordering operator<=>(const const_iterator& other) const { return m_iter <=> other.m_iter; }
	};
	[[nodiscard]] std::string toString() const;
	template<typename Source>
	static LimitedSet<T, capacity> create(const Source& source) { LimitedSet<T, capacity> output; for(const T& value : source) output.insert(value); return output; }
};
// Define custom serialization / deserialization instead of using intrusive because this type is used in raws and specifiying field name would be annoying.
template<typename T, int32_t capacity>
inline void to_json(Json& data, const LimitedSet<T, capacity>& set) { data = set.toJson(); }
template<typename T, int32_t capacity>
inline void from_json(const Json& data, LimitedSet<T, capacity>& set) { set.fromJson(data); }