#pragma once
#include "json.h"
#include <vector>

template <typename Contained, class Index>
class DataVector
{
	std::vector<Contained> data;
public:
	using iterator = std::vector<Contained>::iterator;
	using const_iterator = std::vector<Contained>::const_iterator;
	[[nodiscard]] Contained& operator[](const Index& index) { assert(index < size()); return data[index.get()]; }
	[[nodiscard]] const Contained& operator[](const Index& index) const { assert(index < size()); return data[index.get()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] iterator begin() { return data.begin(); }
	[[nodiscard]] iterator end() { return data.end(); }
	[[nodiscard]] const_iterator begin() const { return data.begin(); }
	[[nodiscard]] const_iterator end() const { return data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] bool contains(const Contained& value) const { return find(value) != end(); }
	[[nodiscard]] bool allAreSetTo(const Contained& value) const { return !std::ranges::any_of(data, [&](Contained d){ return d != value; }); }
	[[nodiscard]] iterator find(const Contained& value) { return std::ranges::find(data, value); }
	[[nodiscard]] const_iterator find(const Contained& value) const { return std::ranges::find(data, value); }
	template<typename Predicate>
	[[nodiscard]] iterator find_if(const Predicate& predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] const_iterator find_if(const Predicate& predicate) const { return std::ranges::find_if(data, predicate); }
	[[nodiscard]] Index indexFor(const Contained& value) const { auto found = find(value); assert(found != end()); return Index::create(found - begin()); }
	[[nodiscard]] auto front() -> Contained& { return data.front(); }
	[[nodiscard]] auto back() -> Contained& { return data.back(); }
	[[nodiscard]] auto front() const -> const Contained& { return data.front(); }
	[[nodiscard]] auto back() const -> const Contained& { return data.back(); }
	void reserve(const size_t& size) { data.reserve(size); }
	void reserve(const Index& size) { data.reserve(size.get()); }
	void resize(const size_t& size) { data.resize(size); }
	void resize(const Index& size) { data.resize(size.get()); }
	void add(const Contained& value) { data.emplace_back(value); }
	void add(Contained&& value) { data.emplace_back(std::move(value)); }
	void add() { data.resize(data.size() + 1); }
	void clear() { data.clear(); }
	void popBack() { data.pop_back(); }
	void remove(const Index& index)
	{
		assert(index < size());
		if(size() == 1)
		{
			clear();
			return;
		}
		Index last = Index::create(size() - 1);
		//TODO: benchmark this branch.
		if(index != last)
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
	[[nodiscard]] bool operator[](const Index& index) const { return data[index.get()]; }
	[[nodiscard]] size_t size() const { return data.size(); }
	void set(const Index& index) { assert(index < size()); data[index.get()] = true; }
	void set(const Index& index, bool status) { assert(index < size()); data[index.get()] = status; }
	void unset(const Index& index) { assert(index < size()); data[index.get()] = false; }
	void reserve(const size_t& size) { data.reserve(size); }
	void reserve(const Index& index) { data.reserve(index.get()); }
	void resize(const size_t& size) { data.resize(size); }
	void resize(const Index& index) { data.resize(index.get()); }
	void clear() { data.clear(); }
	void add(const bool& status) { data.resize(data.size() + 1); data[data.size() - 1] = status; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataBitSet, data);
};
