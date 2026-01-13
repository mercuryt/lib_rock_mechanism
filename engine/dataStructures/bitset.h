#pragma once

#include "../json.h"

template<typename IntType, IntType capacity>
struct BitSet
{
	static_assert(std::is_unsigned<IntType>::value, "IntType must be an unsigned type");
	using This = BitSet<IntType, capacity>;
	constexpr static IntType one = 1;
	constexpr static IntType zero = 0;
	static_assert(sizeof(IntType) >= (capacity / 8));
	IntType data;
	BitSet();
	BitSet(const IntType& value);
	BitSet(const This& other) = default;
	[[nodiscard]] bool operator[](const IntType& index) const;
	[[nodiscard]] bool test(const IntType& index) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool any() const;
	[[nodiscard]] bool operator==(const BitSet<IntType, capacity>& other) const = default;
	[[nodiscard]] std::strong_ordering operator<=>(const BitSet<IntType, capacity>& other) const = default;
	void set(const IntType& index);
	void set(const IntType& index, bool value);
	void unset(const IntType& index);
	void clear();
	void fill();
	void fill(bool value);
	void operator=(IntType d);
	void clearAllAfterInclusive(const IntType& index);
	void clearAllBefore(const IntType& index);
	void flip();
	[[nodiscard]] IntType getNextAndClear();
	[[nodiscard]] IntType getNext();
	[[nodiscard]] IntType getLast();
	[[nodiscard]] BitSet<IntType, capacity> head(const IntType& index) const; // Make a copy, mask everything before inclusive, return.
	[[nodiscard]] BitSet<IntType, capacity> afterInclusive(const IntType& index) const; // Make a copy, mask everything after, return.
	[[nodiscard]] static BitSet<IntType, capacity> create(const IntType& d);
	[[nodiscard]] static BitSet<IntType, capacity> create(const Eigen::Array<bool, 1, 64>& boolArray);
	[[nodiscard]] __attribute__((noinline)) bool testDbg(const IntType& index) const;
	[[nodiscard]] __attribute__((noinline)) std::string toString() const;
	[[nodiscard]] __attribute__((noinline)) int32_t popCount() const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BitSet, data);
};
typedef BitSet<uint64_t, 64u> BitSet64;