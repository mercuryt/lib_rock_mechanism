#pragma once

#include "../json.h"

template<typename IntType, unsigned int capacity>
struct BitSet
{
	using This = BitSet<IntType, capacity>;
	static_assert(sizeof(IntType) >= (capacity / 8));
	IntType data;
	BitSet();
	BitSet(const IntType& value);
	BitSet(const This& other) = default;
	[[nodiscard]] bool operator[](const uint8_t& index) const;
	[[nodiscard]] bool test(const uint8_t& index) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool any() const;
	[[nodiscard]] std::string toString() const;
	[[nodiscard]] bool operator==(const BitSet<IntType, capacity>& other) const = default;
	[[nodiscard]] std::strong_ordering operator<=>(const BitSet<IntType, capacity>& other) const = default;
	[[nodiscard]] __attribute__((noinline)) std::string toString();
	void set(const uint8_t& index);
	void set(const uint8_t& index, bool value);
	void unset(const uint8_t& index);
	void clear();
	void fill();
	void fill(bool value);
	void operator=(IntType d);
	[[nodiscard]] static BitSet create(const IntType& d);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BitSet, data);
};

template<unsigned int capacity>
struct BitSet<
	std::conditional<std::greater(capacity, 32), uint64_t,
		std::conditional<std::greater(capacity, 16), uint32_t,
			std::conditional<std::greater(capacity, 8), uint16_t,
				uint8_t
			>
		>
	>,
	capacity
>
{ };