#include "bitset.h"
// Included to use for toString.
#include <bitset>
#include <limits>

#pragma once

template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity>::BitSet() : data(0) { }
template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity>::BitSet(const IntType& value) : data(value) { }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::operator[](const uint8_t& index) const { return test(index); }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::test(const uint8_t& index) const { assert(index < capacity); return (data >> index) & 1ULL; }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::empty() const { return data == 0; }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::any() const { return data != 0; }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::set(const uint8_t& index) { data |= (1ULL << index); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::set(const uint8_t& index, bool value) { if(value) set(index); else unset(index); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::unset(const uint8_t& index) { data &= ~(1ULL << index); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::clear() { data = 0; }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::fill() { data = std::numeric_limits<IntType>::max(); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::fill(bool value) { if(value) fill(); else clear(); }
template<typename IntType, unsigned int capacity>
void BitSet<IntType, capacity>::operator=(IntType d) { data = d; }
template<typename IntType, unsigned int capacity>
BitSet<IntType, capacity> BitSet<IntType, capacity>::create(const IntType& d) { return {d}; }
template<typename IntType, unsigned int capacity>
bool BitSet<IntType, capacity>::testDbg(const uint8_t& index) const { assert(index < capacity); return (data >> index) & 1ULL; }
template<typename IntType, unsigned int capacity>
std::string BitSet<IntType, capacity>::toString() const { return std::bitset<capacity>(data).to_string(); }