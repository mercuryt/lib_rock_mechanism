#pragma once

#include "json.h"
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <vector>
#include <algorithm>
#include <unordered_set>
class PlantIndex;
class ItemIndex;
class ActorIndex;
class Simulation;
template <typename T, T NULL_VALUE = std::numeric_limits<T>::max()>
class IndexType
{
protected:
	T data;
public:
	IndexType() : data(NULL_VALUE) { }
	IndexType(T d) : data(d) { }
	IndexType(const IndexType<T, NULL_VALUE>& o) { data = o.data; }
	IndexType(const IndexType<T, NULL_VALUE>&& o) { data = o.data; }
	IndexType<T>& operator=(const IndexType<T, NULL_VALUE>& o) { data = o.data; return *this;}
	IndexType<T>& operator=(T& d) { data = d; return *this;}
	[[nodiscard]] T get() const { return data; }
	void set(T d) { data = d; }
	void clear() { data = NULL_VALUE; }
	T operator++() { T d = data; ++data; return d; }
	T operator++(int) { return ++data; }
	void reserve(int size) { data.reserve(size); }
	[[nodiscard]] bool exists() const { return data != NULL_VALUE; }
	[[nodiscard]] bool empty() const { return data == NULL_VALUE; }
	[[nodiscard]] bool operator==(const IndexType<T, NULL_VALUE>& o) const { return o.data == data; }
	[[nodiscard]] std::strong_ordering operator<=>(const IndexType<T, NULL_VALUE>& o) const { return data <=> o.data; }
	[[nodiscard]] T operator()() const { return data; }
	[[nodiscard]] static T null() { return NULL_VALUE; }
	struct Hash { [[nodiscard]] size_t operator()(const IndexType<T>& index) const { return index(); } };
};
class BlockIndex : public IndexType<uint32_t>
{
	BlockIndex(uint32_t index) : IndexType<uint32_t>(index) { }
public:
	static BlockIndex create(uint32_t index){ return BlockIndex(index); }
	[[nodiscard]] static BlockIndex null() { return BlockIndex(); }
	BlockIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BlockIndex& index) const { return index(); } };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlockIndex, data);
};
class VisionFacadeIndex : public IndexType<uint32_t>
{
	VisionFacadeIndex(uint32_t index) : IndexType<uint32_t>(index) { }
public:
	static VisionFacadeIndex create(uint32_t index){ return VisionFacadeIndex(index); }
	[[nodiscard]] static VisionFacadeIndex null() { return VisionFacadeIndex(); }
	VisionFacadeIndex() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(VisionFacadeIndex, data);
};
class PathRequestIndex : public IndexType<uint32_t>
{
	PathRequestIndex(uint32_t index) : IndexType<uint32_t>(index) { }
public:
	static PathRequestIndex create(uint32_t index){ return PathRequestIndex(index); }
	[[nodiscard]] static PathRequestIndex null() { return PathRequestIndex(); }
	PathRequestIndex() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PathRequestIndex, data);
};
class HasShapeIndex : public IndexType<uint32_t>
{
protected:
	HasShapeIndex(uint32_t index) : IndexType<uint32_t>(index) { }
public:
	[[nodiscard]] static HasShapeIndex cast(const PlantIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ItemIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ActorIndex& o);
	[[nodiscard]] static HasShapeIndex null() { return IndexType<uint32_t>::null(); }
	[[nodiscard]] static HasShapeIndex create(uint32_t index) { return index; }
	HasShapeIndex() = default;
	[[nodiscard]] PlantIndex toPlant() const;
	[[nodiscard]] ActorIndex toActor() const;
	[[nodiscard]] ItemIndex toItem() const;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasShapeIndex, data);
};
class PlantIndex final : public HasShapeIndex
{
protected:
	PlantIndex(uint32_t index) : HasShapeIndex(index) { }
public:
	[[nodiscard]] static PlantIndex cast(const HasShapeIndex& o) { return PlantIndex(o()); }
	[[nodiscard]] static PlantIndex null() { return PlantIndex(); }
	[[nodiscard]] static PlantIndex create(uint32_t index) { return index; }
	PlantIndex() = default;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PlantIndex, data);
};
class ItemIndex final : public HasShapeIndex
{
	ItemIndex(uint32_t index) : HasShapeIndex(index) { }
public:
	[[nodiscard]] static ItemIndex cast(const HasShapeIndex& o) { return ItemIndex(o()); }
	[[nodiscard]] static ItemIndex null() { return ItemIndex(); }
	[[nodiscard]] static ItemIndex create(uint32_t index) { return index; }
	ItemIndex() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ItemIndex, data);
};
class ActorIndex final : public HasShapeIndex
{
	ActorIndex(uint32_t index) : HasShapeIndex(index) { }
public:
	[[nodiscard]] static ActorIndex cast(const HasShapeIndex& o) { return ActorIndex(o()); }
	[[nodiscard]] static ActorIndex null() { return ActorIndex(); }
	[[nodiscard]] static ActorIndex create(uint32_t index) { return index; }
	ActorIndex() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActorIndex, data);
};
inline HasShapeIndex HasShapeIndex::cast(const PlantIndex& o) { return HasShapeIndex(o()); }
inline HasShapeIndex HasShapeIndex::cast(const ItemIndex& o) { return HasShapeIndex(o()); }
inline HasShapeIndex HasShapeIndex::cast(const ActorIndex& o) { return HasShapeIndex(o()); }
inline PlantIndex HasShapeIndex::toPlant() const { return PlantIndex::create(get()); }
inline ItemIndex HasShapeIndex::toItem() const { return ItemIndex::create(get()); }
inline ActorIndex HasShapeIndex::toActor() const { return ActorIndex::create(get()); }
template <class IndexType>
class IndicesBase
{
	std::vector<IndexType> data;
public:
	IndicesBase(std::initializer_list<IndexType> i) : data(i) { }
	IndicesBase() = default;
	void add(IndexType index) { assert(!contains(index)); data.push_back(index); }
	template <class T>
	void add(T::iterator begin, T::iterator end) { assert(begin <= end); while(begin != end) add(begin++); }
	void maybeAdd(IndexType index) { if(!contains(index)) data.push_back(index); }
	void remove(IndexType index) { assert(contains(index)); auto found = find(index); (*found) = data.back(); data.resize(data.size() - 1);}
	void update(IndexType oldIndex, IndexType newIndex) { assert(!contains(newIndex)); auto found = find(oldIndex); assert(found != data.end()); (*found) = newIndex;}
	void clear() { data.clear(); }
	void reserve(int size) { data.reserve(size); }
	void swap(IndicesBase<IndexType>& other) { std::swap(data, other.data); }
	template<typename Predicate>
	void erase_if(Predicate predicate) { std::erase_if(data, predicate); }
	void merge(IndicesBase<IndexType>& other) { for(BlockIndex index : other) maybeAdd(index); }
	template<typename Sort>
	void sort(Sort sort) { std::sort(data, sort); }
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] bool contains(IndexType index) const { return find(index) != data.end(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] std::vector<IndexType>::iterator find(IndexType index) { return std::ranges::find(data, index); }
	[[nodiscard]] std::vector<IndexType>::const_iterator find(IndexType index) const { return std::ranges::find(data, index); }
	[[nodiscard]] std::vector<IndexType>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::vector<IndexType>::iterator end() { return data.end(); }
	[[nodiscard]] std::vector<IndexType>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::vector<IndexType>::const_iterator end() const { return data.end(); }
	[[nodiscard]] BlockIndex random(Simulation& simulation) const;
	[[nodiscard]] BlockIndex front() const { return data.front(); }
	[[nodiscard]] BlockIndex back() const { return data.back(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(IndicesBase<IndexType>, data);
	using iterator = std::vector<IndexType>::iterator;
};
using BlockIndices = IndicesBase<BlockIndex>;
using ActorIndices = IndicesBase<ActorIndex>;
using PlantIndices = IndicesBase<PlantIndex>;
using ItemIndices = IndicesBase<ItemIndex>;
using HasShapeIndices = IndicesBase<HasShapeIndex>;
template <class IndexType>
class IndicesSetBase
{
	std::unordered_set<IndexType, typename IndexType::Hash> data;
public:
	void add(IndexType index) { data.insert(index); }
	void remove(IndexType index) { data.erase(index); }
	void clear() { data.clear(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] bool contains(IndexType index) const { return data.contains(index); }
	[[nodiscard]] std::unordered_set<IndexType>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::unordered_set<IndexType>::iterator end() { return data.end(); }
	[[nodiscard]] std::unordered_set<IndexType>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::unordered_set<IndexType>::const_iterator end() const { return data.end(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(IndicesSetBase<IndexType>, data);
};
using BlockIndexSet = IndicesSetBase<BlockIndex>;
using ActorIndexSet = IndicesSetBase<ActorIndex>;
using ItemIndexSet = IndicesSetBase<ItemIndex>;
using PlantIndexSet = IndicesSetBase<PlantIndex>;
using HasShapeIndexSet = IndicesSetBase<HasShapeIndex>;
template <class IndexType, typename T, int count>
class IndicesArrayBase
{
	const static T NULL_VALUE = std::numeric_limits<T>::max();
	using Data = std::array<IndexType, count>;
	Data data;
public:
	void add(IndexType index) { assert(!contains(index)); assert(!full()); auto e = end(); (*e) = index; }
	void maybeAdd(IndexType index) { assert(!full()); if(!contains(index)){ auto e = end(); (*e) = index; } }
	void remove(IndexType index)
       	{
		auto found = find(index);
		assert(found != data.end());
		auto back = end() - 1;
		(*found) = *back;
		(*back) = NULL_VALUE; 
	}
	void update(IndexType oldIndex, IndexType newIndex) { assert(!contains(newIndex)); auto found = find(oldIndex); assert(found != data.end()); (*found) = newIndex;}
	void clear() { assert(!empty()); data.fill(NULL_VALUE); }
	void set(Data d) { data = d; }
	[[nodiscard]] bool contains(IndexType index) const { return find(index) != data.end(); }
	[[nodiscard]] Data::iterator find(IndexType index) { return std::ranges::find(data, index); }
	[[nodiscard]] Data::const_iterator find(IndexType index) const { return std::ranges::find(data, index); }
	[[nodiscard]] Data::iterator begin() { return data.begin(); }
	[[nodiscard]] Data::iterator end() { return std::ranges::find(data, NULL_VALUE); }
	[[nodiscard]] Data::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] Data::const_iterator end() const { return std::ranges::find(data, NULL_VALUE); }
	[[nodiscard]] IndexType back() const { auto e = end(); assert(e != data.begin()); return e - 1; }
	[[nodiscard]] bool full() const { return data.back() != NULL_VALUE; }
	[[nodiscard]] bool empty() const { return data.front() == NULL_VALUE; }
	[[nodiscard]] Data& get() { return data; }
	[[nodiscard]] const Data& get() const { return data; }
};
template <class IndexType, typename T, int count>
inline void to_json(Json& data, const IndicesArrayBase<IndexType, T, count>& array) { data = array.get(); }
template <class IndexType, typename T, int count>
inline void from_json(const Json& data, IndicesArrayBase<IndexType, T, count>& array) { array.set(data.get<std::array<IndexType, count>>()); }
template<int count>
using ActorIndicesArray = IndicesArrayBase<ActorIndex, uint32_t, count>;
template<int count>
using ItemIndicesArray = IndicesArrayBase<ItemIndex, uint32_t, count>;
