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
	[[nodiscard]] const std::vector<Contained>& toVector() const;
	[[nodiscard]] Contained& operator[](const Index& index);
	[[nodiscard]] const Contained& operator[](const Index& index) const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] uint capacity() const;
	[[nodiscard]] iterator begin();
	[[nodiscard]] iterator end();
	[[nodiscard]] const_iterator begin() const;
	[[nodiscard]] const_iterator end() const;
	[[nodiscard]] reverse_iterator rbegin();
	[[nodiscard]] reverse_iterator rend();
	[[nodiscard]] const_reverse_iterator rbegin() const;
	[[nodiscard]] const_reverse_iterator rend() const;
	[[nodiscard]] bool empty() const;
	template<typename Predicate>
	[[nodiscard]] bool contains(const Predicate& predicate) const { return find_if(predicate) != end(); }
	[[nodiscard]] bool contains(const Contained& value) const;
	[[nodiscard]] iterator find(const Contained& value);
	[[nodiscard]] const_iterator find(const Contained& value) const;
	template<typename Predicate>
	[[nodiscard]] iterator find_if(const Predicate& predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] const_iterator find_if(const Predicate& predicate) const { return std::ranges::find_if(data, predicate); }
	[[nodiscard]] Index indexFor(const Contained& value) const;
	[[nodiscard]] auto front() -> Contained& { return data.front(); }
	[[nodiscard]] auto back() -> Contained& { return data.back(); }
	[[nodiscard]] auto front() const -> const Contained& { return data.front(); }
	[[nodiscard]] auto back() const -> const Contained& { return data.back(); }
	void reserve(const size_t& size);
	void reserve(const Index& size);
	void resize(const size_t& size);
	void resize(const Index& size);
	void add(const Contained& value);
	void add(Contained&& value);
	void add();
	template<typename ...Args>
	void emplaceBack(Args&& ...args) { data.emplace_back(std::forward<Args>(args)...); }
	void clear();
	void popBack();
	void remove(const Index& index);
	void remove(iterator iter);
	void remove(iterator iterStart, iterator iterEnd);
	// Erase is a synonm for remove for conformity with vector.
	void erase(iterator iterStart, iterator iterEnd);
	template<typename Condition>
	void removeIf(Condition&& condition)
	{
		iterator iter = std::remove_if(data.begin(), data.end(), condition);
		data.erase(iter, data.end());
	}
	template<typename Comparitor>
	void sortBy(Comparitor&& comparitor) { std::sort(data.begin(), data.end(), comparitor); }
	// Provides symatry with more complex data stores like HasEvents and ReferenceData.
	void moveIndex(const Index& oldIndex, const Index& newIndex);
	void sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<uint32_t, Index>> sortOrder);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(StrongVector, data);
};
template <class Index>
class StrongBitSet
{
	std::vector<bool> data;
public:
	[[nodiscard]] bool operator[](const Index& index) const;
	[[nodiscard]] size_t size() const;
	void set(const Index& index);
	void set(const Index& index, bool status);
	void maybeUnset(const Index& index);
	void unset(const Index& index);
	void reserve(const size_t& size);
	void reserve(const Index& index);
	void resize(const size_t& size);
	void resize(const Index& index);
	void clear();
	void add(const bool& status);
	void moveIndex(const Index& oldIndex, const Index& newIndex);
	void fill(const bool& status);
	void sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<uint32_t, Index>> sortOrder);
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
		[[nodiscard]] Iterator operator++(int) { auto output = *this; ++(*this); return output; }
		Iterator operator++() { auto size = bitSet->size(); assert(offset != size); do ++offset; while(offset != size && !(*bitSet)[offset]); return *this; }
		[[nodiscard]] Index operator*() const { assert(offset != bitSet->size()); return offset; }
		[[nodiscard]] bool operator==(const Iterator& other) const { assert(other.bitSet == bitSet); return other.offset == offset; }
	};
	[[nodiscard]] Iterator begin() const;
	[[nodiscard]] Iterator end() const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(StrongBitSet, data);
};