#pragma once
class Cuboid
{
public:
	BLOCK* m_highest;
	BLOCK* m_lowest;
	Cuboid(BLOCK* h, BLOCK* l);
	Cuboid(BLOCK& h, BLOCK& l);
	Cuboid() : m_highest(nullptr), m_lowest(nullptr) {}
	bool contains(const BLOCK& block) const;
	bool canMerge(const Cuboid& cuboid) const;
	Cuboid sum(const Cuboid& cuboid) const;
	void merge(const Cuboid& cuboid);
	Cuboid getFace(uint32_t faceing) const;
	bool overlapsWith(const Cuboid& cuboid) const;
	size_t size() const;
	bool empty() const { return size() == 0; }
	bool operator==(const Cuboid& cuboid) const;
	struct iterator
	{
		Cuboid* cuboid;
		uint32_t x;
		uint32_t y;
		uint32_t z;

		using difference_type = std::ptrdiff_t;
		using value_type = BLOCK;
		using pointer = BLOCK*;
		using reference = BLOCK&;

		iterator(Cuboid& c, const BLOCK& block);
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
		using value_type = BLOCK;
		using pointer = const BLOCK*;
		using reference = const BLOCK&;

		const_iterator(const Cuboid& c, const BLOCK& block);
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
