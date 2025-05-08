#pragma once
#include "json.h"
#include <vector>

template <typename Contained, class Index>
class StrongVector
{
	std::vector<Contained> data;
public:
	using iterator = std::vector<Contained>::iterator;
	using reverse_iterator = std::vector<Contained>::reverse_iterator;
	using const_iterator = std::vector<Contained>::const_iterator;
	using const_reverse_iterator = std::vector<Contained>::const_reverse_iterator;
	[[nodiscard]] const std::vector<Contained>& toVector() const { return data; }
	[[nodiscard]] Contained& operator[](const Index& index) { assert(index < size()); return data[index.get()]; }
	[[nodiscard]] const Contained& operator[](const Index& index) const { assert(index < size()); return data[index.get()]; }
	[[nodiscard]] uint size() const { return data.size(); }
	[[nodiscard]] uint capacity() const { return data.capacity(); }
	[[nodiscard]] iterator begin() { return data.begin(); }
	[[nodiscard]] iterator end() { return data.end(); }
	[[nodiscard]] const_iterator begin() const { return data.begin(); }
	[[nodiscard]] const_iterator end() const { return data.end(); }
	[[nodiscard]] reverse_iterator rbegin() { return data.rbegin(); }
	[[nodiscard]] reverse_iterator rend() { return data.rend(); }
	[[nodiscard]] const_reverse_iterator rbegin() const { return data.rbegin(); }
	[[nodiscard]] const_reverse_iterator rend() const { return data.rend(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	template<typename Predicate>
	[[nodiscard]] bool contains(const Predicate& predicate) const { return find_if(predicate) != end(); }
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
	template<typename ...Args>
	void emplaceBack(Args&& ...args) { data.emplace_back(std::forward<Args>(args)...); }
	void clear() { data.clear(); }
	void popBack() { data.pop_back(); }
	void remove(const Index& index)
	{
		assert(index < size());
		auto iter = data.begin() + index.get();
		remove(iter);
	}
	void remove(iterator iter)
	{
		assert(iter != end());
		//TODO: benchmark this branch.
		if(iter != end() - 1)
			(*iter) = std::move(*(end() - 1));
		data.resize(data.size() - 1);
	}
	void remove(iterator iterStart, iterator iterEnd)
	{
		assert(iterStart != end());
		data.erase(iterStart, iterEnd);
	}
	// Erase is a synonm for remove for conformity with vector.
	void erase(iterator iterStart, iterator iterEnd) { remove(iterStart, iterEnd); }
	template<typename Condition>
	void removeIf(Condition&& condition)
	{
		iterator iter = std::remove_if(data.begin(), data.end(), condition);
		data.erase(iter, data.end());
	}
	template<typename Comparitor>
	void sortBy(Comparitor&& comparitor) { std::sort(data.begin(), data.end(), comparitor); }
	// Provides symatry with more complex data stores like HasEvents and ReferenceData.
	void moveIndex(const Index& oldIndex, const Index& newIndex) { data[newIndex.get()] = std::move(data[oldIndex.get()]); }
	void sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<uint32_t, Index>> sortOrder)
	{
		std::vector<Contained> copy;
		copy.reserve((end - begin).get());
		std::ranges::move(data.begin() + begin.get(), data.begin() + end.get(), std::back_inserter(copy));
		for(Index index = begin; index < end; ++index)
		{
			auto offset = (index + begin).get();
			Index from = sortOrder[offset].second;
			data[offset] = std::move(copy[from.get()]);
		}
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(StrongVector, data);
};
template <class Index>
class StrongBitSet
{
	std::vector<bool> data;
public:
	[[nodiscard]] bool operator[](const Index& index) const { assert((uint)index.get() < data.size()); return data[index.get()]; }
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
	void moveIndex(const Index& oldIndex, const Index& newIndex) { data[newIndex.get()] = data[oldIndex.get()]; }
	void fill(const bool& status) { std::fill(data.begin(), data.end(), status); }
	void sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<uint32_t, Index>> sortOrder)
	{
		std::vector<bool> copy;
		copy.reserve((end - begin).get());
		std::ranges::move(data.begin() + begin.get(), data.begin() + end.get(), std::back_inserter(copy));
		for(Index index = begin; index < end; ++index)
		{
			auto offset = (index + begin).get();
			Index from = sortOrder[offset].second;
			data[offset] = std::move(copy[from.get()]);
		}
	}
	template<typename Action>
	void forEach(Action&& action)
	{
		for(auto index = Index::create(0); index < data.size(); ++index)
			if(data[index.get()])
				action(index);
	};
	struct Iterator
	{
		const StrongBitSet* bitSet;
		Index offset;
		[[nodiscard]] Iterator operator++(int){ auto output = this; ++(*this); return output; }
		Iterator operator++(){ auto size = bitSet->size(); assert(offset != size); do ++offset; while(offset != size && !(*bitSet)[offset]); return *this; }
		[[nodiscard]] Index operator*() const { assert(offset != bitSet->size()); return offset; }
		[[nodiscard]] bool operator==(const Iterator& other) const { assert(other.bitSet == bitSet); return other.offset == offset; }
	};
	[[nodiscard]] Iterator begin() const { return Iterator(this, Index::create(0)); }
	[[nodiscard]] Iterator end() const { return Iterator(this, Index::create(size())); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(StrongBitSet, data);
};