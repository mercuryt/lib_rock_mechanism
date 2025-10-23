#pragma once
#include "json.h"
#include "concepts.h"
#include <compare>
#include <limits>
#include <concepts>
class Simulation;
template <class Derived, typename T, T NULL_VALUE = std::numeric_limits<T>::max(), T MIN_VALUE = std::numeric_limits<T>::min()>
struct StrongInteger
{
	T data = NULL_VALUE;
	using This = StrongInteger<Derived, T, NULL_VALUE, MIN_VALUE>;
	constexpr static T MAX_VALUE = NULL_VALUE - 1;
	using Primitive = T;
	constexpr Derived& operator=(const This& o) { data = o.data; return static_cast<Derived&>(*this); }
	constexpr Derived& operator=(const T& d) { data = d; return static_cast<Derived&>(*this); }
	constexpr void set(T d) { data = d; }
	// TODO: add maybeClear.
	constexpr void clear() { data = NULL_VALUE; }
	constexpr Derived& operator+=(const This& other) { return (*this) += other.data; }
	constexpr Derived& operator-=(const This& other) { return (*this) -= other.data; }
	template<Numeric Other>
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
	template<Numeric Other>
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
	template<Numeric Other>
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
	template<Numeric Other>
	constexpr Derived& operator/=(const Other& other) { assert(other != 0); assert(exists()); data /= other; return static_cast<Derived&>(*this); }
	constexpr Derived& operator/=(const This& other) { assert(other.exists()); assert(other != 0); (*this) /= other.data; }
	[[nodiscard]] constexpr Derived operator-() const { assert(exists()); return Derived::create(-data); }
	[[nodiscard]] constexpr T get() const { return data; }
	[[nodiscard]] T& getReference() { return data; }
	[[nodiscard]] constexpr static Derived create(const T& d){ Derived der; der.set(d); return der; }
	[[nodiscard]] constexpr static Derived null() { return Derived::create(NULL_VALUE); }
	[[nodiscard]] constexpr static Derived max() { return Derived::create(NULL_VALUE - 1); }
	[[nodiscard]] constexpr static Derived min() { return Derived::create(MIN_VALUE); }
	[[nodiscard]] constexpr Derived absoluteValue() { assert(exists()); return Derived::create(std::abs(data)); }
	[[nodiscard]] constexpr bool exists() const { return data != NULL_VALUE; }
	[[nodiscard]] constexpr bool empty() const { return data == NULL_VALUE; }
	[[nodiscard]] constexpr bool modulusIsZero(const This& other) const { assert(exists()); return data % other.data == 0;  }
	[[nodiscard]] constexpr Derived operator++(int) { assert(exists()); assert(data != NULL_VALUE); T d = data; ++data; return Derived::create(d); }
	[[nodiscard]] constexpr Derived operator--(int) { assert(exists()); assert(data != MIN_VALUE); T d = data; --data; return Derived::create(d); }
	[[nodiscard]] constexpr bool operator==(const This& other) const { return other.data == data; }
	[[nodiscard]] constexpr bool operator!=(const This& other) const { return other.data != data; }
	template<Numeric Other>
	[[nodiscard]] constexpr bool operator==(const Other& other) const { return (T)other == data; }
	template<Numeric Other>
	[[nodiscard]] constexpr bool operator!=(const Other& other) const { return (T)other != data; }
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const This& o) const { assert(exists()); assert(o.exists()); return data <=> o.data; }
	template<Numeric Other>
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const Other& other) const { return data <=> (T)other; }
	[[nodiscard]] constexpr std::string toString() const { return std::to_string(data); }
	[[nodiscard]] constexpr Derived operator+(const This& other) const { return (*this) + other.data; }
	template<Numeric Other>
	[[nodiscard]] constexpr Derived operator+(const Other& other) const {
		assert(exists());
		if(other < 0)
			assert(MIN_VALUE - other <= data);
		else
			assert(MAX_VALUE - other >= data);
		return Derived::create(data + other);
	}
	[[nodiscard]] constexpr Derived operator-(const This& other) const { return (*this) - other.data; }
	template<Numeric Other>
	[[nodiscard]] constexpr Derived operator-(const Other& other) const {
		assert(exists());
		if(other < 0)
			assert(MAX_VALUE + other >= data);
		else
			assert(MIN_VALUE + other <= data);
		return Derived::create(data - other);
	}
	[[nodiscard]] constexpr Derived operator*(const This& other) const { return (*this) * other.data; }
	template<Numeric Other>
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
				if constexpr (std::is_signed_v<T>)
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
	template<Numeric Other>
	[[nodiscard]] constexpr Derived operator/(const Other& other) const { assert(exists()); return Derived::create(data / other); }
	template<Numeric Other>
	[[nodiscard]] constexpr Derived operator%(const Other& other) const { assert(exists()); return Derived::create(data % other); }
	[[nodiscard]] constexpr Derived subtractWithMinimum(const This& other) const { assert(other.exists()); return subtractWithMinimum(other.data); }
	template<Numeric Other>
	[[nodiscard]] constexpr Derived subtractWithMinimum(const Other& other) const
	{
		assert(exists());
		if(other < 0)
			return addWithMaximum(other * -1);
		int result = (int)data - (int)other;
		return Derived::create(result < (int)MIN_VALUE ? MIN_VALUE : result);
	}
	[[nodiscard]] constexpr Derived addWithMaximum(const This& other) const { assert(other.exists()); return addWithMaximum(other.data); }
	template<Numeric Other>
	[[nodiscard]] constexpr Derived addWithMaximum(const Other& other) const
	{
		assert(exists());
		if(other < 0)
			return subtractWithMinimum(other * -1);
		static_assert(MAX_VALUE < INT64_MAX);
		int64_t result = (int64_t)data + (int64_t)other;
		return Derived::create(result > (int64_t)MAX_VALUE ? MAX_VALUE : result);
	}
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const This& index) const { return index.get(); } };
};