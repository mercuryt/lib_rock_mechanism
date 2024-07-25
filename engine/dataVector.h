#pragma once
#include "../lib/dynamic_bitset.hpp"
#include "json.h"
#include <vector>

template <typename T, class Index>
class DataVector
{
	std::vector<T> data;
public:
	using iterator = std::vector<T>::iterator;
	using const_iterator = std::vector<T>::const_iterator;
	[[nodiscard]] T& at(Index index) { return data[index()]; }
	[[nodiscard]] const T& at(Index index) const { return data[index()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] iterator begin() { return data.begin(); }
	[[nodiscard]] iterator end() { return data.end(); }
	[[nodiscard]] const_iterator begin() const { return data.begin(); }
	[[nodiscard]] const_iterator end() const { return data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	void resize(Index size) { data.resize(size.get()); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataVector, data);
};

template <class Index>
class DataBitSet
{
	sul::dynamic_bitset<> data;
public:
	[[nodiscard]] bool at(Index index) const { return data[index()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	void set(Index index) { data.set(index, true); }
	void set(Index index, bool status) { data.set(index, status); }
	void unset(Index index) { data.set(index(), false); }
	void resize(int size) { data.resize(size); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataBitSet, data);
};
