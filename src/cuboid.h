#pragma once
template<class DerivedBlock, class DerivedActor, class DerivedArea>
class BaseCuboid
{
public:
	DerivedBlock* m_highest;
	DerivedBlock* m_lowest;

	BaseCuboid(DerivedBlock* h, DerivedBlock* l);
	BaseCuboid(DerivedBlock& h, DerivedBlock& l);
	BaseCuboid() : m_highest(nullptr), m_lowest(nullptr) {}
	bool contains(const DerivedBlock& block) const;
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
		using value_type = DerivedBlock;
		using pointer = DerivedBlock*;
		using reference = DerivedBlock&;

		iterator(BaseCuboid& c, const DerivedBlock& block);
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
		using value_type = DerivedBlock;
		using pointer = const DerivedBlock*;
		using reference = const DerivedBlock&;

		const_iterator(const BaseCuboid& c, const DerivedBlock& block);
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
