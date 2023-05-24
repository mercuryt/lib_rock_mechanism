#pragma once
template<class Block>
class BaseCuboid
{
public:
	Block* m_highest;
	Block* m_lowest;

	BaseCuboid(Block* h, Block* l);
	BaseCuboid(Block& h, Block& l);
	BaseCuboid() : m_highest(nullptr), m_lowest(nullptr) {}
	bool contains(const Block& block) const;
	bool canMerge(const BaseCuboid& cuboid) const;
	BaseCuboid sum(const BaseCuboid& cuboid) const;
	void merge(const BaseCuboid& cuboid);
	BaseCuboid getFace(uint32_t faceing) const;
	bool overlapsWith(const BaseCuboid& cuboid) const;
	size_t size() const;
	bool empty() const { return size() == 0; }
	bool operator==(const BaseCuboid& cuboid) const;
	struct iterator
	{
		BaseCuboid* cuboid;
		uint32_t x;
		uint32_t y;
		uint32_t z;

		using difference_type = std::ptrdiff_t;
		using value_type = Block;
		using pointer = Block*;
		using reference = Block&;

		iterator(BaseCuboid& c, const Block& block);
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
		const BaseCuboid* cuboid;
		uint32_t x;
		uint32_t y;
		uint32_t z;

		using difference_type = std::ptrdiff_t;
		using value_type = Block;
		using pointer = const Block*;
		using reference = const Block&;

		const_iterator(const BaseCuboid& c, const Block& block);
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
