#pragma once
#include "types.h"
#include "json.h"
#include <cassert>
#include <initializer_list>
#include <vector>
#include <ranges>
class Simulation;
class BlockIndices
{
	std::vector<BlockIndex> data;
public:
	BlockIndices() = default;
	BlockIndices(std::initializer_list<BlockIndex> initalizer) : data(initalizer) { }
	void add(BlockIndex block) { assert(!contains(block)); data.push_back(block); }
	void maybeAdd(BlockIndex block) { if(!contains(block)) data.push_back(block); }
	void remove(BlockIndex block) { assert(contains(block)); auto found = find(block); (*found) = data.back(); data.resize(data.size() - 1); }
	void clear() { data.clear(); }
	void reserve(int size) { data.reserve(size); }
	void swap(BlockIndices& other) { std::swap(data, other.data); }
	template<typename Predicate>
	void erase_if(Predicate predicate) { std::erase_if(data, predicate); }
	void merge(BlockIndices& other) { for(BlockIndex index : other) maybeAdd(index); }
	template<typename Sort>
	void sort(Sort sort) { std::sort(data, sort); }
	[[nodiscard]] std::vector<BlockIndex>::iterator find(BlockIndex block) { return std::ranges::find(data, block); }
	[[nodiscard]] std::vector<BlockIndex>::const_iterator find(BlockIndex block) const { return std::ranges::find(data, block); }
	[[nodiscard]] std::vector<BlockIndex>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::vector<BlockIndex>::iterator end() { return data.end(); }
	[[nodiscard]] std::vector<BlockIndex>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::vector<BlockIndex>::const_iterator end() const { return data.end(); }
	[[nodiscard]] bool contains(BlockIndex block) const { return find(block) != data.end(); } 
	[[nodiscard]] bool empty() const { return data.empty(); } 
	[[nodiscard]] size_t size() const { return data.size(); }
	[[nodiscard]] BlockIndex random(Simulation& simulation) const;
	[[nodiscard]] BlockIndex front() const { return data.front(); }
	[[nodiscard]] BlockIndex back() const { return data.back(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlockIndices, data);
	using iterator = std::vector<BlockIndex>::iterator;
	using const_iterator = std::vector<BlockIndex>::const_iterator;
};
