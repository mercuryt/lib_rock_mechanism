#include "bitset.h"
// Included to use for toString.
#include <bitset>
#include <limits>

#include <immintrin.h> // Required for AVX2 intrinsics


#pragma once

template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity>::BitSet() : data(0) { }
template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity>::BitSet(const IntType& value) : data(value) { }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::operator[](const uint8_t& index) const { return test(index); }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::test(const uint8_t& index) const { assert(index < capacity); return (data >> index) & std::integral_constant<IntType, 1>::value; }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::empty() const { return data == 0; }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::any() const { return data != 0; }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::set(const uint8_t& index) { data |= (std::integral_constant<IntType, 1>::value << index); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::set(const uint8_t& index, bool value) { if(value) set(index); else unset(index); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::unset(const uint8_t& index) { data &= ~(std::integral_constant<IntType, 1>::value << index); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::clear() { data = 0; }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::fill() { data = std::numeric_limits<IntType>::max(); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::fill(bool value) { if(value) fill(); else clear(); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::operator=(IntType d) { data = d; }
template<typename IntType, unsigned int capacity>
uint8_t BitSet<IntType, capacity>::getNextAndClear()
{
	uint8_t next = getNext();
	unset(next);
	return next;
}
template<typename IntType, unsigned int capacity>
uint8_t BitSet<IntType, capacity>::getNext() { return std::countr_zero(data); }
template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::create(const IntType& d) { return {d}; }
template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::create(const Eigen::Array<bool, 1, 64>& booleanArray)
{
	if constexpr(64 != capacity)
	{
		assert(false);
		std::unreachable();
	}
	// Eigen::Array<bool> stores bools densely as bytes under the hood.
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(booleanArray.data());
	// load the two halves
	__m256i v0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
	__m256i v1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr + 32));
	// Convert 1 to 255. 0 is unchanged.
	__m256i fullByteV0 = _mm256_cmpgt_epi8(v0, _mm256_setzero_si256());
	__m256i fullByteV1 = _mm256_cmpgt_epi8(v1, _mm256_setzero_si256());
	// Create 32 bit masks.
	uint32_t mask0 = static_cast<uint32_t>(_mm256_movemask_epi8(fullByteV0));
	uint32_t mask1 = static_cast<uint32_t>(_mm256_movemask_epi8(fullByteV1));
	// Merge masks.
	return (static_cast<uint64_t>(mask1) << 32) | mask0;
}
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::testDbg(const uint8_t& index) const { assert(index < capacity); return (data >> index) & std::integral_constant<IntType, 1>::value; }
template<typename IntType, unsigned int capacity>
std::string BitSet<IntType, capacity>::toString() const { return std::bitset<capacity>(data).to_string(); }
template<typename IntType, unsigned int capacity>
uint BitSet<IntType, capacity>::popCount() const { return __builtin_popcountll(data); }