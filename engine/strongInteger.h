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
	constexpr StrongInteger(const T& d) : data(d) { }
	constexpr StrongInteger(const This& o) { data = o.data; }
	constexpr StrongInteger(const This&& o) { data = o.data; }
	constexpr Derived& operator=(const This& o) { data = o.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator=(const T& d) { data = d; return static_cast<Derived&>(*this); }
	constexpr void set(T d) { data = d; }
	// TODO: add maybeClear.
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
	constexpr Derived& operator/=(const Other& other) { assert(other != 0); assert(exists()); data /= other; return static_cast<Derived&>(*this); }
	constexpr Derived& operator/=(const This& other) { assert(other.exists()); assert(other != 0); (*this) /= other.data; }
	[[nodiscard]] constexpr Derived operator-() const { assert(exists()); return Derived::create(-data); }
	[[nodiscard]] constexpr T get() const { return data; }
	[[nodiscard]] T& getReference() { return data; }
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
	[[nodiscard]] constexpr std::string toString() const { return std::to_string(data); }
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
		if(other < 0)
			assert(MAX_VALUE + other >= data);
		else
			assert(MIN_VALUE + other <= data);
		return Derived::create(data - other);
	}
	[[nodiscard]] constexpr Derived operator*(const This& other) const { return (*this) * other.data; }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator*(const Other& other) const {
		assert(exists());
		bool skipValidation = false;
		if constexpr (std::signed_integral<T>)
		{
			if(std::abs(other) <= 1)
				skipValidation = true;
		}
		else
			if(other <= 1)
				skipValidation = true;
		if(!skipValidation)
		{
			if((other > 0 && data > 0) || (other < 0 && data < 0) )
			{
				auto maxValue = MAX_VALUE;
				assert(maxValue > 0);
				T fraction;
				if constexpr (std::is_signed_v<Other>)
					fraction = maxValue / std::abs(other);
				else
					fraction = maxValue / other;
				assert(fraction > data);
			}
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
		}
		return Derived::create(data * other);
	}
	[[nodiscard]] constexpr Derived operator/(const This& other) const { return (*this) / other.data; }
	[[nodiscard]] constexpr Derived operator%(const This& other) const { return (*this) % other.data; }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator/(const Other& other) const { assert(exists()); return Derived::create(data / other); }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived operator%(const Other& other) const { assert(exists()); return Derived::create(data % other); }
	[[nodiscard]] constexpr Derived subtractWithMinimum(const This& other) const { assert(other.exists()); return subtractWithMinimum(other.data); }
	template<arithmetic Other>
	[[nodiscard]] constexpr Derived subtractWithMinimum(const Other& other) const
	{
		assert(exists());
		int result = (int)data - (int)other;
		return Derived::create(result < (int)MIN_VALUE ? MIN_VALUE : result);
	}
	// A no inline version of create for use in debug console.
	[[nodiscard]] static __attribute__((noinline)) Derived dbg(const T& value) { return create(value); }
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const This& index) const { return index.get(); } };
};
// TODO: Phase out in favor of SmallSet.
template <class StrongInteger>
class StrongIntegerSet
{
	std::vector<StrongInteger> data;
public:
	StrongIntegerSet(std::initializer_list<StrongInteger> i) : data(i) { }
	StrongIntegerSet() = default;
	void add(const StrongInteger& index) { assert(index.exists()); assert(!contains(index)); data.push_back(index); }
	void addAllowNull(const StrongInteger& index) { if(index.exists()) assert(!contains(index)); data.push_back(index); }
	template <class T>
	void add(T&& begin, T&& end) { assert(begin <= end); while(begin != end) { maybeAdd(*begin); ++begin; } }
	void maybeAdd(const StrongInteger& index) { assert(index.exists()); if(!contains(index)) data.push_back(index); }
	void addNonunique(const StrongInteger& index) { data.push_back(index); }
	void pushFrontNonunique(const StrongInteger& index) { data.insert(data.begin(), index); }
	void remove(const StrongInteger& index) { assert(index.exists()); assert(contains(index)); remove(find(index)); }
	void remove(std::vector<StrongInteger>::iterator iter) { (*iter) = data.back(); data.pop_back(); }
	template <class Predicate>
	void remove_if(Predicate&& predicate) { std::ranges::remove_if(data, predicate); }
	void maybeRemove(const StrongInteger& index) { assert(index.exists()); auto found = find(index); if(found != data.end()) remove(found); }
	void popBack() { data.pop_back(); }
	void update(const StrongInteger& oldIndex, const StrongInteger& newIndex) { assert(oldIndex.exists()); assert(newIndex.exists()); assert(!contains(newIndex)); auto found = find(oldIndex); assert(found != data.end()); (*found) = newIndex;}
	void clear() { data.clear(); }
	void reserve(const int& size) { data.reserve(size); }
	void swap(StrongIntegerSet<StrongInteger>& other) { std::swap(data, other.data); }
	template<typename Predicate>
	void erase_if(Predicate&& predicate) { std::erase_if(data, predicate); }
	void merge(const StrongIntegerSet<StrongInteger>& other) { for(StrongInteger index : other) maybeAdd(index); }
	template <class T>
	void merge(const T& other) { add(other.begin(), other.end()); }
	template<typename Sort>
	void sort(Sort&& sort) { std::ranges::sort(data, sort); }
	template<typename Target>
	void copy(const Target& target) { std::ranges::copy(data, target); }
	template<typename Predicate, typename Target>
	void copy_if(const Target&& target, const Predicate&& predicate) { std::ranges::copy_if(data, target, predicate); }
	void removeDuplicatesAndValue(const StrongInteger& index);
	void removeDuplicates() { std::sort(data.begin(), data.end()); data.erase(std::unique(data.begin(), data.end()), data.end()); }
	void concatAssertUnique(const StrongIntegerSet<StrongInteger>&& other) { for(const auto& item : other.data) add(item); }
	void concatIgnoreUnique(const StrongIntegerSet<StrongInteger>&& other) { for(const auto& item : other.data) maybeAdd(item); }
	void updateValue(const StrongInteger& oldValue, const StrongInteger& newValue) { assert(!contains(newValue)); auto found = find(oldValue); assert(found != data.end()); (*found) = newValue; }
	void unique() { std::ranges::sort(data); data.erase(std::ranges::unique(data).begin(), data.end()); }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] bool contains(const StrongInteger& index) const { return find(index) != data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] std::vector<StrongInteger>::iterator find(StrongInteger index) { return std::ranges::find(data, index); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator find(StrongInteger target) const { return std::ranges::find(data, target); }
	[[nodiscard]] uint findLastIndex(StrongInteger target) const
	{
		uint output = 0;
		const auto& size = data.size();
		for(; output < size; ++output)
			if(data[output] == target)
				break;
		return output;
	}
	template<typename Predicate>
	[[nodiscard]] std::vector<StrongInteger>::iterator find_if(Predicate predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] std::vector<StrongInteger>::const_iterator find_if(Predicate predicate) const { return std::ranges::find_if(data, predicate); }
	[[nodiscard]] std::vector<StrongInteger>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::vector<StrongInteger>::iterator end() { return data.end(); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator end() const { return data.end(); }
	[[nodiscard]] StrongInteger random(Simulation& simulation) const;
	[[nodiscard]] StrongInteger front() const { assert(!data.empty()); return data.front(); }
	[[nodiscard]] StrongInteger back() const { return data.back(); }
	[[nodiscard]] StrongInteger operator[](const uint& i) const { assert(i < size()); return data[i]; }
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
