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
	[[nodiscard]] T& at(Index index) { return data[index.get()]; }
	[[nodiscard]] const T& at(Index index) const { return data[index.get()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] iterator begin() { return data.begin(); }
	[[nodiscard]] iterator end() { return data.end(); }
	[[nodiscard]] const_iterator begin() const { return data.begin(); }
	[[nodiscard]] const_iterator end() const { return data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] bool allAreSetTo(T value) const { return !std::ranges::any_of(data, [&](T d){ return d != value; }); }
	[[nodiscard]] iterator find(T value) { return std::ranges::find(data, value); }
	[[nodiscard]] const_iterator find(T value) const { return std::ranges::find(data, value); }
	void reserve(size_t size) { data.reserve(size); }
	void reserve(Index size) { data.reserve(size.get()); }
	void resize(size_t size) { data.resize(size); }
	void resize(Index size) { data.resize(size.get()); }
	void add(T value) { data.push_back(value); }
	void add() { data.resize(data.size() + 1); }
	void clear() { data.clear(); }
	void remove(Index index)
	{
		assert(index < size());
		if(size() == 1)
		{
			clear();
			return;
		}
		Index last = Index::create(size() - 1);
		data.at(index.get()) = std::move(data.at(last.get()));
		data.resize(data.size() - 1);
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataVector, data);
};

template <class Index>
class DataBitSet
{
	sul::dynamic_bitset<> data;
public:
	[[nodiscard]] bool at(Index index) const { return data[index.get()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	void set(Index index) { data.set(index.get(), 1, true); }
	void set(Index index, bool status) { data.set(index.get(), 1, status); }
	void unset(Index index) { data.set(index.get(), 1, false); }
	void reserve(size_t size) { data.reserve(size); }
	void reserve(Index index) { data.reserve(index.get()); }
	void resize(size_t size) { data.resize(size); }
	void resize(Index index) { data.resize(index.get()); }
	void clear() { data.clear(); }
	void add(bool status) { data.resize(data.size() + 1); data.set(data.size() - 1, 1, status); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataBitSet, data);
};
