#pragma once
#include "json.h"
#include <compare>
#include <limits>
class Simulation;
template <class Derived, typename T, T NULL_VALUE = std::numeric_limits<T>::max()>
class StrongInteger
{
protected:
	T data;
public:
	constexpr StrongInteger() : data(NULL_VALUE) { }
	constexpr StrongInteger(T d) : data(d) { }
	constexpr StrongInteger(const StrongInteger<Derived, T, NULL_VALUE>& o) { data = o.data; }
	constexpr StrongInteger(const StrongInteger<Derived, T, NULL_VALUE>&& o) { data = o.data; }
	constexpr auto operator=(const StrongInteger<Derived, T, NULL_VALUE>& o) { data = o.data; return static_cast<Derived&>(*this); }
	constexpr auto operator=(const T& d) { data = d; return static_cast<Derived&>(*this); }
	constexpr void set(T d) { data = d; }
	constexpr void clear() { data = NULL_VALUE; }
	constexpr void reserve(int size) { data.reserve(size); }
	constexpr auto operator+=(const StrongInteger<Derived, T, NULL_VALUE>& other) { data += other.data; return static_cast<Derived&>(*this); }
	constexpr auto operator-=(const StrongInteger<Derived, T, NULL_VALUE>& other) { data -= other.data; return static_cast<Derived&>(*this); }
	constexpr auto operator+=(T other) { data += other; return static_cast<Derived&>(*this); }
	constexpr auto operator-=(T other) { data -= other; return static_cast<Derived&>(*this); }
	constexpr auto operator++() { ++data; return static_cast<Derived&>(*this); }
	constexpr auto operator--() { --data; return static_cast<Derived&>(*this); }
	constexpr auto operator-() const { return Derived::create(-data); }
	constexpr auto operator*=(T other) { data *= other; return static_cast<Derived&>(*this); }
	constexpr auto operator*=(const StrongInteger<Derived, T, NULL_VALUE>& other) { data *= other.data; return static_cast<Derived&>(*this); }
	constexpr auto operator/=(T other) { data *= other; return static_cast<Derived&>(*this); }
	constexpr auto operator/=(const StrongInteger<Derived, T, NULL_VALUE>& other) { data *= other.data; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr T get() const { return data; }
	[[nodiscard]] constexpr static auto create(T d){ Derived der; der.set(d); return der; }
	[[nodiscard]] constexpr static auto null() { return Derived::create(NULL_VALUE); }
	[[nodiscard]] constexpr auto absoluteValue() { return Derived::create(std::abs(data)); }
	[[nodiscard]] constexpr auto operator++(int) { T d = data; ++data; return Derived::create(d); }
	[[nodiscard]] constexpr auto operator--(int) { T d = data; --data; return Derived::create(d); }
	[[nodiscard]] constexpr bool operator==(const StrongInteger<Derived, T, NULL_VALUE>& o) const { return o.data == data; }
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const StrongInteger<Derived, T, NULL_VALUE>& o) const { return data <=> o.data; }
	[[nodiscard]] constexpr auto operator+(const StrongInteger<Derived, T, NULL_VALUE>& other) const { return Derived::create(data + other.data); }
	[[nodiscard]] constexpr auto operator+(const T& other) const { return Derived::create(data + other); }
	[[nodiscard]] constexpr auto operator-(const StrongInteger<Derived, T, NULL_VALUE>& other) const { return Derived::create(data - other.data); }
	[[nodiscard]] constexpr auto operator-(const T& other) const { return Derived::create(data - other); }
	[[nodiscard]] constexpr auto operator*(const StrongInteger<Derived, T, NULL_VALUE>& other) const { return Derived::create(data * other.data); }
	[[nodiscard]] constexpr auto operator*(const T& other) const { return Derived::create(data * other); }
	[[nodiscard]] constexpr auto operator/(const float& other) const { return Derived::create(data / other); }
	[[nodiscard]] constexpr auto operator/(const StrongInteger<Derived, T, NULL_VALUE>& other) const { return Derived::create(data / other.data); }
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const StrongInteger<Derived, T, NULL_VALUE>& index) const { return index.get(); } };
	[[nodiscard]] constexpr bool exists() const { return data != NULL_VALUE; }
	[[nodiscard]] constexpr bool empty() const { return data == NULL_VALUE; }
	[[nodiscard]] constexpr bool modulusIsZero(const StrongInteger<Derived, T, NULL_VALUE>& other) const { return data % other.data == 0;  }
};
template <class StrongInteger>
class StrongIntegerSet
{
	std::vector<StrongInteger> data;
public:
	StrongIntegerSet(std::initializer_list<StrongInteger> i) : data(i) { }
	StrongIntegerSet() = default;
	void add(StrongInteger index) { assert(!contains(index)); data.push_back(index); }
	template <class T>
	void add(T&& begin, T&& end) { assert(begin <= end); while(begin != end) { maybeAdd(*begin); ++begin; } }
	void maybeAdd(StrongInteger index) { if(!contains(index)) data.push_back(index); }
	void remove(StrongInteger index) { assert(contains(index)); auto found = find(index); (*found) = data.back(); data.resize(data.size() - 1);}
	template <class Predicate>
	void remove_if(Predicate&& predicate) { std::ranges::remove_if(data, predicate); }
	void update(StrongInteger oldIndex, StrongInteger newIndex) { assert(!contains(newIndex)); auto found = find(oldIndex); assert(found != data.end()); (*found) = newIndex;}
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
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] bool contains(StrongInteger index) const { return find(index) != data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] std::vector<StrongInteger>::iterator find(StrongInteger index) { return std::ranges::find(data, index); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator find(StrongInteger index) const { return std::ranges::find(data, index); }
	[[nodiscard]] std::vector<StrongInteger>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::vector<StrongInteger>::iterator end() { return data.end(); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::vector<StrongInteger>::const_iterator end() const { return data.end(); }
	[[nodiscard]] StrongInteger random(Simulation& simulation) const;
	[[nodiscard]] StrongInteger front() const { return data.front(); }
	[[nodiscard]] StrongInteger back() const { return data.back(); }
	[[nodiscard]] StrongInteger at(uint i) const { return data.at(i); }
	[[nodiscard]] auto back_inserter() { return std::back_inserter(data); }
	[[nodiscard]] bool operator==(const StrongIntegerSet<StrongInteger>& other) { return this == &other; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(StrongIntegerSet<StrongInteger>, data);
	using iterator = std::vector<StrongInteger>::iterator;
};
