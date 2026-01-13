#pragma once
#include "strongVector.h"
#include <concepts>
#include <type_traits>
template<typename Contained, class Index>
const std::vector<Contained>& StrongVector<Contained, Index>::toVector() const { return data; }
template<typename Contained, class Index>
Contained& StrongVector<Contained, Index>::operator[](const Index& index) { assert(index < size()); return data[index.get()]; }
template<typename Contained, class Index>
const Contained& StrongVector<Contained, Index>::operator[](const Index& index) const { assert(index < size()); return data[index.get()]; }
template<typename Contained, class Index>
int32_t StrongVector<Contained, Index>::size() const { return data.size(); }
template<typename Contained, class Index>
int32_t StrongVector<Contained, Index>::capacity() const { return data.capacity(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::iterator StrongVector<Contained, Index>::begin() { return data.begin(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::iterator StrongVector<Contained, Index>::end() { return data.end(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::const_iterator StrongVector<Contained, Index>::begin() const { return data.begin(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::const_iterator StrongVector<Contained, Index>::end() const { return data.end(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::reverse_iterator StrongVector<Contained, Index>::rbegin() { return data.rbegin(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::reverse_iterator StrongVector<Contained, Index>::rend() { return data.rend(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::const_reverse_iterator StrongVector<Contained, Index>::rbegin() const { return data.rbegin(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::const_reverse_iterator StrongVector<Contained, Index>::rend() const { return data.rend(); }
template<typename Contained, class Index>
bool StrongVector<Contained, Index>::empty() const { return data.empty(); }
template<typename Contained, class Index>
bool StrongVector<Contained, Index>::contains(const Contained& value) const { return find(value) != end(); }
template<typename Contained, class Index>
StrongVector<Contained, Index>::iterator StrongVector<Contained, Index>::find(const Contained& value)
{
	if constexpr(std::equality_comparable<Contained>)
		return std::ranges::find(data, value);
	else
		assert(false);
}
template<typename Contained, class Index>
StrongVector<Contained, Index>::const_iterator StrongVector<Contained, Index>::find(const Contained& value) const { return const_cast<StrongVector<Contained, Index>*>(this)->find(value); }
template<typename Contained, class Index>
Index StrongVector<Contained, Index>::indexFor(const Contained& value) const
{
	const auto found = find(value);
	assert(found != end());
	return Index::create(found - begin());
}

template<typename Contained, class Index>
void StrongVector<Contained, Index>::reserve(const size_t& size) { data.reserve(size); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::reserve(const Index& size) { data.reserve(size.get()); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::resize(const size_t& size) { data.resize(size); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::resize(const Index& size) { data.resize(size.get()); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::add(const Contained& value)
{
	// TODO: Use concepts instead of type_traits?
	if constexpr(std::is_copy_constructible_v<Contained>)
		data.emplace_back(value);
	else
		assert(false);
}
template<typename Contained, class Index>
void StrongVector<Contained, Index>::add(Contained&& value) { data.emplace_back(std::move(value)); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::add() { data.resize(data.size() + 1); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::clear() { data.clear(); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::popBack() { data.pop_back(); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::remove(const Index& index)
{
	assert(index < size());
	auto iter = data.begin() + index.get();
	remove(iter);
}
template<typename Contained, class Index>
void StrongVector<Contained, Index>::remove(iterator iter)
{
	assert(iter != end());
	//TODO: benchmark this branch.
	if(iter != end() - 1)
		(*iter) = std::move(*(end() - 1));
	data.resize(data.size() - 1);
}
template<typename Contained, class Index>
void StrongVector<Contained, Index>::remove(iterator iterStart, iterator iterEnd)
{
	assert(iterStart != end());
	data.erase(iterStart, iterEnd);
}
template<typename Contained, class Index>
void StrongVector<Contained, Index>::erase(iterator iterStart, iterator iterEnd) { remove(iterStart, iterEnd); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::moveIndex(const Index& oldIndex, const Index& newIndex) { data[newIndex.get()] = std::move(data[oldIndex.get()]); }
template<typename Contained, class Index>
void StrongVector<Contained, Index>::sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<int32_t, Index>> sortOrder)
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
template<class Index>
bool StrongBitSet<Index>::operator[](const Index& index) const { assert((size_t)index.get() < data.size()); return data[index.get()]; }
template<class Index>
size_t StrongBitSet<Index>::size() const { return data.size(); }
template<class Index>
void StrongBitSet<Index>::set(const Index& index) { assert(index < size()); data[index.get()] = true; }
template<class Index>
void StrongBitSet<Index>::set(const Index& index, bool status) { assert(index < size()); data[index.get()] = status; }
template<class Index>
void StrongBitSet<Index>::maybeUnset(const Index& index) { assert(index < size()); data[index.get()] = false; }
template<class Index>
void StrongBitSet<Index>::unset(const Index& index) { assert(index < size()); assert(data[index.get()]); data[index.get()] = false; }
template<class Index>
void StrongBitSet<Index>::reserve(const size_t& size) { data.reserve(size); }
template<class Index>
void StrongBitSet<Index>::reserve(const Index& index) { data.reserve(index.get()); }
template<class Index>
void StrongBitSet<Index>::resize(const size_t& size) { data.resize(size); }
template<class Index>
void StrongBitSet<Index>::resize(const Index& index) { data.resize(index.get()); }
template<class Index>
void StrongBitSet<Index>::clear() { data.clear(); }
template<class Index>
void StrongBitSet<Index>::add(const bool& status) { data.resize(data.size() + 1); data[data.size() - 1] = status; }
template<class Index>
void StrongBitSet<Index>::moveIndex(const Index& oldIndex, const Index& newIndex) { data[newIndex.get()] = data[oldIndex.get()]; }
template<class Index>
void StrongBitSet<Index>::fill(const bool& status) { std::fill(data.begin(), data.end(), status); }
template<class Index>
void StrongBitSet<Index>::sortRangeWithOrder(const Index& begin, const Index& end, std::vector<std::pair<int32_t, Index>> sortOrder)
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
template<class Index>
StrongBitSet<Index>::Iterator StrongBitSet<Index>::begin() const { return Iterator(this, Index::create(0)); }
template<class Index>
StrongBitSet<Index>::Iterator StrongBitSet<Index>::end() const { return Iterator(this, Index::create(size())); }