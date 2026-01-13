#include "bitset.h"
// Included to use for toString.
#include <bitset>
#include <limits>

#if defined(__AVX512F__) || defined(__AVX2__)
  #include <immintrin.h>
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
  #include <arm_neon.h>
#endif

#if defined(__wasm_simd128__)
  #include <wasm_simd128.h>
#endif

#pragma once

template<typename IntType, IntType capacity>
BitSet<IntType, capacity>::BitSet() : data(zero) { }
template<typename IntType, IntType capacity>
BitSet<IntType, capacity>::BitSet(const IntType& value) : data(value) { }
template<typename IntType, IntType capacity>
bool BitSet<IntType, capacity>::operator[](const IntType& index) const { return test(index); }
template<typename IntType, IntType capacity>
bool BitSet<IntType, capacity>::test(const IntType& index) const { assert(index < capacity); return (data >> index) & one; }
template<typename IntType, IntType capacity>
bool BitSet<IntType, capacity>::empty() const { return data == zero; }
template<typename IntType, IntType capacity>
bool BitSet<IntType, capacity>::any() const { return data != zero; }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::set(const IntType& index) { assert(index < capacity); data |= (one << index); }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::set(const IntType& index, bool value) { assert(index < capacity); if(value) set(index); else unset(index); }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::unset(const IntType& index) { assert(index < capacity); data &= ~(one << index); }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::clear() { data = zero; }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::fill() { data = std::numeric_limits<IntType>::max(); }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::fill(bool value) { if(value) fill(); else clear(); }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::operator=(IntType d) { data = d; }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::flip() { data = ~data; }
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::clearAllAfterInclusive(const IntType& index)
{
	assert(index < capacity);
	const IntType mask = (one << index) - one;
	data &= mask;
}
template<typename IntType, IntType capacity>
void BitSet<IntType, capacity>::clearAllBefore(const IntType& index)
{
	assert(index <= capacity);
	const IntType mask = ~zero << index;
	data &= mask;
}
template<typename IntType, IntType capacity>
IntType BitSet<IntType, capacity>::getNextAndClear()
{
	IntType next = getNext();
	unset(next);
	return next;
}
template<typename IntType, IntType capacity>
IntType BitSet<IntType, capacity>::getNext() { return std::countr_zero(data); }
template<typename IntType, IntType capacity>
IntType BitSet<IntType, capacity>::getLast() { assert(!empty()); return (capacity - 1) - std::countl_zero(data); }
template<typename IntType, IntType capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::head(const IntType& index) const
{
	BitSet output = *this;
	output.clearAllAfterInclusive(index);
	return output;
}
template<typename IntType, IntType capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::afterInclusive(const IntType& index) const
{
	BitSet output = *this;
	output.clearAllBefore(index);
	return output;
}
template<typename IntType, IntType capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::create(const IntType& d) { return {d}; }
template<typename IntType, IntType capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::create(const Eigen::Array<bool, 1, 64>& booleanArray)
{
	if constexpr(64u != capacity)
	{
		assert(false);
		std::unreachable();
	}
	// Eigen::Array<bool> stores bools densely as bytes under the hood.
	const uint8_t* ptr = reinterpret_cast<const uint8_t*>(booleanArray.data());
	#if defined(__AVX512F__)
		// ---- AVX-512 path ----
		// Load 64 bytes
		__m512i v = _mm512_loadu_si512(ptr);

		// Compare > 0 gives a 64-bit mask directly
		__mmask64 k = _mm512_cmpgt_epi8_mask(v, _mm512_setzero_si512());

		return static_cast<uint64_t>(k);
	#elif defined(__AVX2__)
		const __m256i zero = _mm256_setzero_si256();
		// load the two halves
		__m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
		__m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + 32));
		// Convert 1 to 255. 0 is unchanged.
		__m256i fullByteV0 = _mm256_cmpgt_epi8(v0, zero);
		__m256i fullByteV1 = _mm256_cmpgt_epi8(v1, zero);
		// Create 32 bit masks.
		uint32_t mask0 = _mm256_movemask_epi8(fullByteV0);
		uint32_t mask1 = _mm256_movemask_epi8(fullByteV1);
		// Merge masks.
		return (static_cast<uint64_t>(mask1) << 32) | static_cast<uint64_t>(mask0);
	#elif defined(__aarch64__)
		// ---------- ARM NEON (AArch64) ----------
		// NEON has no movemask instruction; we synthesize it.
		uint8x16_t zero = vdupq_n_u8(0);
		uint8x16_t v0 = vld1q_u8(ptr +  0);
		uint8x16_t v1 = vld1q_u8(ptr + 16);
		uint8x16_t v2 = vld1q_u8(ptr + 32);
		uint8x16_t v3 = vld1q_u8(ptr + 48);
		// Compare > 0 → 0xFF / 0x00
		uint8x16_t c0 = vcgtq_u8(v0, zero);
		uint8x16_t c1 = vcgtq_u8(v1, zero);
		uint8x16_t c2 = vcgtq_u8(v2, zero);
		uint8x16_t c3 = vcgtq_u8(v3, zero);
		// Extract MSBs
		uint8x16_t bits0 = vshrq_n_u8(c0, 7);
		uint8x16_t bits1 = vshrq_n_u8(c1, 7);
		uint8x16_t bits2 = vshrq_n_u8(c2, 7);
		uint8x16_t bits3 = vshrq_n_u8(c3, 7);
		// Pack to scalars
		uint64_t mask = 0;
		for (int i = 0; i < 16; ++i) mask |= uint64_t(vgetq_lane_u8(bits0, i)) << (i +  0);
		for (int i = 0; i < 16; ++i) mask |= uint64_t(vgetq_lane_u8(bits1, i)) << (i + 16);
		for (int i = 0; i < 16; ++i) mask |= uint64_t(vgetq_lane_u8(bits2, i)) << (i + 32);
		for (int i = 0; i < 16; ++i) mask |= uint64_t(vgetq_lane_u8(bits3, i)) << (i + 48);
		return mask;
	#elif defined(__wasm_simd128__)
		// ---------- WASM SIMD ----------
		// WASM has i8x16 comparisons but no movemask → same issue as NEON.
		v128_t zero = wasm_i8x16_splat(0);
		v128_t v0 = wasm_v128_load(ptr +  0);
		v128_t v1 = wasm_v128_load(ptr + 16);
		v128_t v2 = wasm_v128_load(ptr + 32);
		v128_t v3 = wasm_v128_load(ptr + 48);
		v128_t c0 = wasm_i8x16_gt(v0, zero);
		v128_t c1 = wasm_i8x16_gt(v1, zero);
		v128_t c2 = wasm_i8x16_gt(v2, zero);
		v128_t c3 = wasm_i8x16_gt(v3, zero);
		// Shift MSB into bit 0
		c0 = wasm_u8x16_shr(c0, 7);
		c1 = wasm_u8x16_shr(c1, 7);
		c2 = wasm_u8x16_shr(c2, 7);
		c3 = wasm_u8x16_shr(c3, 7);
		uint64_t mask = 0;
		for (int i = 0; i < 16; ++i) mask |= uint64_t(wasm_u8x16_extract_lane(c0, i)) << (i +  0);
		for (int i = 0; i < 16; ++i) mask |= uint64_t(wasm_u8x16_extract_lane(c1, i)) << (i + 16);
		for (int i = 0; i < 16; ++i) mask |= uint64_t(wasm_u8x16_extract_lane(c2, i)) << (i + 32);
		for (int i = 0; i < 16; ++i) mask |= uint64_t(wasm_u8x16_extract_lane(c3, i)) << (i + 48);
		return mask;
	#else
		// ---------- Scalar fallback ----------
		uint64_t mask = 0;
		for (int i = 0; i != 64; ++i)
			mask |= uint64_t(ptr[i] != 0) << i;
		return mask;
	#endif
}
template<typename IntType, IntType capacity>
bool BitSet<IntType, capacity>::testDbg(const IntType& index) const { assert(index < capacity); return (data >> index) & one; }
template<typename IntType, IntType capacity>
std::string BitSet<IntType, capacity>::toString() const { return std::bitset<capacity>(data).to_string(); }
template<typename IntType, IntType capacity>
int BitSet<IntType, capacity>::popCount() const { return __builtin_popcountll(data); }
