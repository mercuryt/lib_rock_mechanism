#pragma once
#include "json.h"

#include <limits>
#include <compare>
#include <float.h>

template <class Derived>
struct StrongFloat
{
	float data = FLT_MAX;
	using Primitive = float;
	[[nodiscard]] constexpr static Derived create(const float d) { Derived der; der.set(d); return der; }
	[[nodiscard]] constexpr static Derived null() { return create(FLT_MAX); }
	[[nodiscard]] constexpr static float nullPrimitive() { return FLT_MAX; }
	[[nodiscard]] constexpr bool exists() const { return data != FLT_MAX; }
	[[nodiscard]] constexpr bool empty() const { return data == FLT_MAX; }
	[[nodiscard]] constexpr float get() const { return data; }
	// 10^32 is the smallest value that can be subtracted from FLT_MAX and produce a number distinct from FLT_MAX.
	[[nodiscard]] constexpr static Derived max() { return create(FLT_MAX - std::pow(10, 32)); }
	[[nodiscard]] constexpr static Derived min() { return create(FLT_MIN); }
	constexpr void set(const float d ) { data = d; }
	constexpr void clear() { data = FLT_MAX; }
	constexpr Derived& operator=(const float d) { data = d; return static_cast<Derived&>(*this); }
	//constexpr Derived& operator=(const StrongFloat<Derived> d) { data = d.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator++() { assert(exists()); ++data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator--() { assert(exists()); --data; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr Derived operator++(int) { assert(exists()); auto d = data; ++data; return d; }
	[[nodiscard]] constexpr Derived operator--(int) { assert(exists()); auto d = data; --data; return d; }
	constexpr Derived& operator+=(const float other) { assert(exists()); data += other; return static_cast<Derived&>(*this); }
	constexpr Derived& operator-=(const float other) { assert(exists()); data -= other; return static_cast<Derived&>(*this); }
	constexpr Derived& operator*=(const float other) { assert(exists()); data *= other; return static_cast<Derived&>(*this); }
	constexpr Derived& operator/=(const float other) { assert(exists()); data /= other; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr Derived operator+(const float other) const { assert(exists()); return create(data + other); }
	[[nodiscard]] constexpr Derived operator-(const float other) const { assert(exists()); return create(data - other); }
	[[nodiscard]] constexpr Derived operator*(const float other) const { assert(exists()); return create(data * other); }
	[[nodiscard]] constexpr Derived operator/(const float other) const { assert(exists()); return create(data / other); }
	[[nodiscard]] constexpr Derived operator-() const { assert(exists()); return create(-data); }
	[[nodiscard]] constexpr bool operator==(const float other) const { assert(exists()); return data == other; }
	[[nodiscard]] constexpr bool operator!=(const float other) const { assert(exists()); return data != other; }
	[[nodiscard]] constexpr std::partial_ordering operator<=>(const float other) const { assert(exists()); return data <=> other; }
	constexpr Derived& operator+=(const StrongFloat<Derived> other) { assert(exists()); assert(other.exists()); data += other.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator-=(const StrongFloat<Derived> other) { assert(exists()); assert(other.exists()); data -= other.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator*=(const StrongFloat<Derived> other) { assert(exists()); assert(other.exists()); data *= other.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator/=(const StrongFloat<Derived> other) { assert(exists()); assert(other.exists()); data /= other.data; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr Derived operator+(const StrongFloat<Derived> other) const { assert(exists()); assert(other.exists()); return create(data + other.data); }
	[[nodiscard]] constexpr Derived operator-(const StrongFloat<Derived> other) const { assert(exists()); assert(other.exists()); return create(data - other.data); }
	[[nodiscard]] constexpr Derived operator*(const StrongFloat<Derived> other) const { assert(exists()); assert(other.exists()); return create(data * other.data); }
	[[nodiscard]] constexpr Derived operator/(const StrongFloat<Derived> other) const { assert(exists()); assert(other.exists()); return create(data / other.data); }
	[[nodiscard]] constexpr bool operator==(const StrongFloat<Derived> other) const { assert(exists()); assert(other.exists()); return data == other.data; }
	[[nodiscard]] constexpr bool operator!=(const StrongFloat<Derived> other) const { return data != other.data; }
	[[nodiscard]] constexpr std::partial_ordering operator<=>(const StrongFloat<Derived> other) const { return data <=> other.data; }
};
