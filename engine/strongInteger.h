#pragma once
#include "json.h"
#include <compare>
#include <limits>
#include <concepts>

template<typename T>
concept arithmetic = std::integral<T> or std::floating_point<T>;
class Simulation;
template <class Derived, typename T, T NULL_VALUE = std::numeric_limits<T>::max(), T MIN_VALUE = std::numeric_limits<T>::min()>
class StrongInteger
{
protected:
	T data;
public:
	using This = StrongInteger<Derived, T, NULL_VALUE>;
	constexpr static T MAX_VALUE = NULL_VALUE - 1;
	constexpr StrongInteger() : data(NULL_VALUE) { }
	constexpr StrongInteger(T d) : data(d) { }
	constexpr StrongInteger(const This& o) { data = o.data; }
	constexpr StrongInteger(const This&& o) { data = o.data; }
	constexpr Derived& operator=(const This& o) { data = o.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator=(const T& d) { data = d; return static_cast<Derived&>(*this); }
	constexpr void set(T d) { data = d; }
	constexpr void clear() { data = NULL_VALUE; }
	constexpr Derived& operator+=(const This& other) { return (*this) += other.data; }
	constexpr Derived& operator-=(const This& other) { return (*this) -= other.data; }
	template<arithmetic Other>
	constexpr Derived& operator+=(const Other& other)
	{
		assert(exists());
		if(other < 0)
			assert(MIN_VALUE - other <= data);
		else
			assert(MAX_VALUE - other >= data);
		data += other;
		return static_cast<Derived&>(*this);
	}
	template<arithmetic Other>
	constexpr Derived& operator-=(const Other& other)
	{
		assert(exists());
		if(other < 0)
			assert(MAX_VALUE - other <= data);
		else
			assert(MIN_VALUE + other <= data);
		data -= other;
		return static_cast<Derived&>(*this);
	}
	constexpr Derived& operator++() { assert(exists()); assert(data != NULL_VALUE - 1); ++data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator--() { assert(exists()); assert(data != MIN_VALUE); --data; return static_cast<Derived&>(*this); }
	template<arithmetic Other>
	constexpr Derived& operator*=(const Other& other)
	{
		assert(exists());
		if((other > 0 && data > 0) || (other < 0 && data < 0) )
			assert(MAX_VALUE / other >= data);
		else if(other < 0) // other is negitive and data is positive.
		{
			assert(MIN_VALUE != 0);
			// MIN_VALUE divided by -1 returns MIN_VALUE, strangely.
			if constexpr(std::signed_integral<T>) if(other != -1)
				assert(MIN_VALUE / other >= data);
		}
		else if(other > 0)// other is positive and data is negitive.
			assert(MIN_VALUE / other <= data);
		data *= other;
		return static_cast<Derived&>(*this);
	}
	constexpr Derived& operator*=(const This& other) { return (*this) *= other.data; }
	template<arithmetic Other>
	constexpr Derived& operator/=(const Other& other) { assert(exists()); data /= other; return static_cast<Derived&>(*this); }
	constexpr Derived& operator/=(const This& other) { (*this) /= other.data; }
	[[nodiscard]] constexpr Derived operator-() const { assert(exists()); return Derived::create(-data); }
	[[nodiscard]] constexpr T get() const { assert(exists()); return data; }
	[[nodiscard]] T& getReference() { assert(exists()); return data; }
	[[nodiscard]] constexpr static Derived create(const T& d){ Derived der; der.set(d); return der; }
	[[nodiscard]] constexpr static Derived create(const T&& d){ return create(d); }
	[[nodiscard]] constexpr static Derived null() { return Derived::create(NULL_VALUE); }
	[[nodiscard]] constexpr static Derived max() { return Derived::create(NULL_VALUE - 1); }
	[[nodiscard]] constexpr static Derived min() { return Derived::create(MIN_VALUE); }
	[[nodiscard]] constexpr Derived absoluteValue() { assert(exists()); return Derived::create(std::abs(data)); }
	[[nodiscard]] constexpr bool exists() const { return data != NULL_VALUE; }
	[[nodiscard]] constexpr bool empty() const { return data == NULL_VALUE; }
	[[nodiscard]] constexpr bool modulusIsZero(const This& other) const { assert(exists()); return data % other.data == 0;  }
	[[nodiscard]] constexpr Derived operator++(int) { assert(exists()); assert(data != NULL_VALUE); T d = data; ++data; return Derived::create(d); }
	[[nodiscard]] constexpr Derived operator--(int) { assert(exists()); assert(data != MIN_VALUE); T d = data; --data; return Derived::create(d); }
	[[nodiscard]] constexpr bool operator==(const This& o) const { /*assert(exists()); assert(o.exists());*/ return o.data == data; }
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const StrongInteger<Derived, T, NULL_VALUE>& o) const { assert(exists()); assert(o.exists()); return data <=> o.data; }
	[[nodiscard]] constexpr Derived operator+(const This& other) const { return (*this) + other.data; }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator+(const Other& other) const {
		assert(exists());
		if(other < 0)
			assert(MIN_VALUE + other >= data);
		else
			assert(MAX_VALUE - other >= data);
		return Derived::create(data + other);
	}
	[[nodiscard]] constexpr Derived operator-(const This& other) const { return (*this) - other.data; }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator-(const Other& other) const {
		assert(exists());
		assert(MIN_VALUE + other <= data);
		if(other < 0)
			assert(MAX_VALUE - other <= data);
		else
			assert(MIN_VALUE + other <= data);
		return Derived::create(data - other);
	}
	[[nodiscard]] constexpr Derived operator*(const This& other) const { return (*this) * other.data; }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator*(const Other& other) const {
		assert(exists());
		if((other > 0 && data > 0) || (other < 0 && data < 0) )
			assert(MAX_VALUE / other >= data);
		else if(other < 0 ) // other is negitive and data is positive.
		{
			if(data != 0)
				assert(MIN_VALUE != 0);
			// MIN_VALUE divided by -1 returns MIN_VALUE, strangely.
			if constexpr(std::signed_integral<T>) if(other != -1)
				assert(MIN_VALUE / other >= data);
		}
		else if(other > 0)// other is positive and data is negitive.
			assert(MIN_VALUE / other <= data);
		return Derived::create(data * other);
	}
	[[nodiscard]] constexpr Derived operator/(const This& other) const { return (*this) / other.data; }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator/(const Other& other) const { assert(exists()); return Derived::create(data / other); }
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const This& index) const { return index.get(); } };
};
template <class StrongInteger>
class StrongIntegerSet
{
	std::vector<StrongInteger> data;
public:
	StrongIntegerSet(std::initializer_list<StrongInteger> i) : data(i) { }
	StrongIntegerSet() = default;
	void add(StrongInteger index) { assert(index.exists()); assert(!contains(index)); data.push_back(index); }
	void addAllowNull(StrongInteger index) { if(index.exists()) assert(!contains(index)); data.push_back(index); }
	template <class T>
	void add(T&& begin, T&& end) { assert(begin <= end); while(begin != end) { maybeAdd(*begin); ++begin; } }
	void maybeAdd(StrongInteger index) { assert(index.exists()); if(!contains(index)) data.push_back(index); }
	void addNonunique(StrongInteger index) { data.push_back(index); }
	void remove(StrongInteger index) { assert(index.exists()); assert(contains(index)); remove(find(index)); }
	void remove(std::vector<StrongInteger>::iterator iter) { (*iter) = data.back(); data.pop_back(); }
	template <class Predicate>
	void remove_if(Predicate&& predicate) { std::ranges::remove_if(data, predicate); }
	void maybeRemove(StrongInteger index) { assert(index.exists()); auto found = find(index); if(found != data.end()) remove(found); }
	void update(StrongInteger oldIndex, StrongInteger newIndex) { assert(oldIndex.exists()); assert(newIndex.exists()); assert(!contains(newIndex)); auto found = find(oldIndex); assert(found != data.end()); (*found) = newIndex;}
	void clear() { data.clear(); }
	void reserve(int size) { data.reserve(size); }
	void swap(StrongIntegerSet<StrongInteger>& other) { std::swap(data, other.data); }
	template<typename Predicate>
	void erase_if(Predicate&& predicate) { std::erase_if(data, predicate); }
	void merge(StrongIntegerSet<StrongInteger>& other) { for(StrongInteger index : other) maybeAdd(index); }
	template <class T>
	void merge(T& other) { add(other.begin(), other.end()); }
	template<typename Sort>
	void sort(Sort&& sort) { std::ranges::sort(data, sort); }
	template<typename Target>
	void copy(Target& target) { std::ranges::copy(data, target); }
	template<typename Predicate, typename Target>
	void copy_if(Target&& target, Predicate&& predicate) { std::ranges::copy_if(data, target, predicate); }
	void removeDuplicatesAndValue(StrongInteger index);
	void concatAssertUnique(StrongIntegerSet<StrongInteger>&& other) { for(const auto& item : other.data) add(item); }
	void concatIgnoreUnique(StrongIntegerSet<StrongInteger>&& other) { for(const auto& item : other.data) maybeAdd(item); }
	void updateValue(StrongInteger oldValue, StrongInteger newValue) { assert(!contains(newValue)); auto found = find(oldValue); assert(found != data.end()); (*found) = newValue; }
	void unique() { std::ranges::sort(data); std::ranges::unique(data); }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] bool contains(StrongInteger index) const { return find(index) != data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] std::vector<StrongInteger>::iterator find(StrongInteger index) { return std::ranges::find(data, index); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator find(StrongInteger index) const { return std::ranges::find(data, index); }
	template<typename Predicate>
	[[nodiscard]] std::vector<StrongInteger>::iterator find_if(Predicate predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] std::vector<StrongInteger>::const_iterator find_if(Predicate predicate) const { return std::ranges::find_if(data, predicate); }
	[[nodiscard]] std::vector<StrongInteger>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::vector<StrongInteger>::iterator end() { return data.end(); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator end() const { return data.end(); }
	[[nodiscard]] StrongInteger random(Simulation& simulation) const;
	[[nodiscard]] StrongInteger front() const { return data.front(); }
	[[nodiscard]] StrongInteger back() const { return data.back(); }
	[[nodiscard]] StrongInteger operator[](uint i) const { assert(i < size()); return data[i]; }
	[[nodiscard]] auto back_inserter() { return std::back_inserter(data); }
	[[nodiscard]] bool operator==(const StrongIntegerSet<StrongInteger>& other) { return this == &other; }
	template<uint arraySize>
	[[nodiscard]] auto toArray()
	{
		assert(arraySize >= size());
		std::array<StrongInteger, arraySize> output;
		std::ranges::copy(data, output);
		return output;
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(StrongIntegerSet<StrongInteger>, data);
	using iterator = std::vector<StrongInteger>::iterator;
};
