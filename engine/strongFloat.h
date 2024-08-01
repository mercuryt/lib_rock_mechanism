#pragma once
#include "json.h"

#include <limits>
#include <compare>
#include <float.h>

template <class Derived>
class StrongFloat
{
protected:
	float data;
public:
	constexpr static Derived create(float d) { Derived der; der.set(d); return der; }
	constexpr static Derived null() { return create(FLT_MAX); }
	[[nodiscard]] constexpr bool exists() { return data != FLT_MAX; }
	[[nodiscard]] constexpr bool empty() { return data == FLT_MAX; }
	[[nodiscard]] constexpr float get() const { return data; }
	constexpr void set(float d ) { data = d; }
	constexpr auto operator++() { ++data; return static_cast<Derived&>(*this); }
	constexpr auto operator--() { --data; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr auto operator++(int) { auto d = data; ++data; return Derived::create(d); }
	[[nodiscard]] constexpr auto operator--(int) {  auto d = data; --data; return Derived::create(d); }
	constexpr auto operator+=(float other) { data += other; return static_cast<Derived&>(*this); }
	constexpr auto operator-=(float other) { data -= other; return static_cast<Derived&>(*this); }
	constexpr auto operator*=(float other) { data *= other; return static_cast<Derived&>(*this); }
	constexpr auto operator/=(float other) { data /= other; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr auto operator+(float other) const { return Derived::create(data + other); }
	[[nodiscard]] constexpr auto operator-(float other) const { return Derived::create(data - other); }
	[[nodiscard]] constexpr auto operator*(float other) const { return Derived::create(data * other); }
	[[nodiscard]] constexpr auto operator/(float other) const { return Derived::create(data / other); }
	[[nodiscard]] constexpr bool operator==(float other) const { return data == other; }
	[[nodiscard]] constexpr bool operator!=(float other) const { return data != other; }
	[[nodiscard]] constexpr std::partial_ordering operator<=>(float other) const { return data <=> other; }
	constexpr auto operator+=(const StrongFloat<Derived> other) { data += other.data; return static_cast<Derived&>(*this); }
	constexpr auto operator-=(const StrongFloat<Derived> other) { data -= other.data; return static_cast<Derived&>(*this); }
	constexpr auto operator*=(const StrongFloat<Derived> other) { data *= other.data; return static_cast<Derived&>(*this); }
	constexpr auto operator/=(const StrongFloat<Derived> other) { data /= other.data; return static_cast<Derived&>(*this); }
	[[nodiscard]] constexpr auto operator+(const StrongFloat<Derived> other) const { return Derived::create(data + other.data); }
	[[nodiscard]] constexpr auto operator-(const StrongFloat<Derived> other) const { return Derived::create(data - other.data); }
	[[nodiscard]] constexpr auto operator*(const StrongFloat<Derived> other) const { return Derived::create(data * other.data); }
	[[nodiscard]] constexpr auto operator/(const StrongFloat<Derived> other) const { return Derived::create(data / other.data); }
	[[nodiscard]] constexpr bool operator==(const StrongFloat<Derived> other) const { return data == other.data; }
	[[nodiscard]] constexpr bool operator!=(const StrongFloat<Derived> other) const { return data != other.data; }
	[[nodiscard]] constexpr std::partial_ordering operator<=>(const StrongFloat<Derived> other) const { return data <=> other.data; }
};
