#pragma once
#include "json.h"
#include <vector>

template <typename Contained, class Index>
class DataVector
{
	std::vector<Contained> data;
public:
	using index = Index;
	using iterator = std::vector<Contained>::iterator;
	using const_iterator = std::vector<Contained>::const_iterator;
	[[nodiscard]] Contained& operator[](Index index) { assert(index < size()); return data[index.get()]; }
	[[nodiscard]] const Contained& operator[](Index index) const { assert(index < size()); return data[index.get()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] iterator begin() { return data.begin(); }
	[[nodiscard]] iterator end() { return data.end(); }
	[[nodiscard]] const_iterator begin() const { return data.begin(); }
	[[nodiscard]] const_iterator end() const { return data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] bool allAreSetTo(Contained value) const { return !std::ranges::any_of(data, [&](Contained d){ return d != value; }); }
	[[nodiscard]] iterator find(Contained value) { return std::ranges::find(data, value); }
	[[nodiscard]] const_iterator find(Contained value) const { return std::ranges::find(data, value); }
	template<typename Predicate>
	[[nodiscard]] iterator find_if(Predicate predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] const_iterator find_if(Predicate predicate) const { return std::ranges::find_if(data, predicate); }
	[[nodiscard]] Index indexFor(Contained value) const { assert(contains(value)); return find(value) - begin(); }
	void reserve(size_t size) { data.reserve(size); }
	void reserve(Index size) { data.reserve(size.get()); }
	void resize(size_t size) { data.resize(size); }
	void resize(Index size) { data.resize(size.get()); }
	void add(Contained value) { data.emplace_back(value); }
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
		data[index.get()] = std::move(data[last.get()]);
		data.resize(data.size() - 1);
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataVector, data);
};

template <class Index>
class DataBitSet
{
	std::vector<bool> data;
public:
	[[nodiscard]] bool operator[](Index index) const { return data[index.get()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	void set(Index index) { assert(index < size()); data[index.get()] = true; }
	void set(Index index, bool status) { assert(index < size()); data[index.get()] = status; }
	void unset(Index index) { assert(index < size()); data[index.get()] = false; }
	void reserve(size_t size) { data.reserve(size); }
	void reserve(Index index) { data.reserve(index.get()); }
	void resize(size_t size) { data.resize(size); }
	void resize(Index index) { data.resize(index.get()); }
	void clear() { data.clear(); }
	void add(bool status) { data.resize(data.size() + 1); data[data.size() - 1] = status; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataBitSet, data);
};