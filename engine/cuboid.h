#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>
#include <unordered_set>

class Block;

class Cuboid
{
public:
	Block* m_highest;
	Block* m_lowest;

	Cuboid(Block* h, Block* l);
	Cuboid(Block& h, Block& l);
	Cuboid() : m_highest(nullptr), m_lowest(nullptr) {}
	void merge(const Cuboid& cuboid);
	void setFrom(Block& block);
	void setFrom(Block& a, Block& b);
	void clear();
	[[nodiscard]] std::unordered_set<Block*> toSet();
	[[nodiscard]] bool contains(const Block& block) const;
	[[nodiscard]] bool canMerge(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(uint32_t faceing) const;
	[[nodiscard]] bool overlapsWith(const Cuboid& cuboid) const;
	[[nodiscard]] size_t size() const;
	[[nodiscard]] bool empty() const { return size() == 0; }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] static Cuboid fromBlock(Block& block);
	[[nodiscard]] static Cuboid fromBlockPair(Block& a, Block& b);
	struct iterator
	{
		Cuboid* cuboid;
		uint32_t x;
		uint32_t y;
		uint32_t z;

		using difference_type = std::ptrdiff_t;
		using value_type = Block;
		using pointer = Block*;
		using reference = Block&;

		iterator(Cuboid& c, const Block& block);
		iterator();
		iterator& operator++();
		iterator operator++(int) const;
		bool operator==(const iterator other) const;
		bool operator!=(const iterator other) const;
		reference operator*() const;
		pointer operator->() const;
	};
	iterator begin();
	iterator end();
	static_assert(std::forward_iterator<iterator>);
	struct const_iterator
	{
		const Cuboid* cuboid;
		uint32_t x;
		uint32_t y;
		uint32_t z;

		using difference_type = std::ptrdiff_t;
		using value_type = Block;
		using pointer = const Block*;
		using reference = const Block&;

		const_iterator(const Cuboid& c, const Block& block);
		const_iterator();
		const_iterator& operator++();
		const_iterator operator++(int) const;
		bool operator==(const const_iterator other) const;
		bool operator!=(const const_iterator other) const;
		reference operator*() const;
		pointer operator->() const;
	};
	const_iterator begin() const;
	const_iterator end() const;
	static_assert(std::forward_iterator<const_iterator>);
};
