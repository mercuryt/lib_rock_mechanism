#pragma once
#include "../numericTypes/index.h"
#include "../dataStructures/smallSet.h"

template<typename T>
class ThreadStripedWatermarkingStack
{
	int m_start;
	// This could cause false sharing but it should be rare due to buffers stabilizing in size.
	inline static std::vector<std::vector<T>> s_buffer;
	static std::vector<T>& get();
public:
	ThreadStripedWatermarkingStack();
	~ThreadStripedWatermarkingStack();
	[[nodiscard]] T operator[](int) const;
	[[nodiscard]] T& operator[](int);
	[[nodiscard]] T popAndReturnBack();
	[[nodiscard]] T back();
	[[nodiscard]] T front();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool contains(const T& value) const;
	[[nodiscard]] int size() const;
	void insert(T);
	void popBack();
	void clear();
	struct iterator
	{
		int m_index;
		[[nodiscard]] T operator*() const;
		[[nodiscard]] T& operator*();
		[[nodiscard]] bool operator==(const iterator& other) const = default;
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const = default;
		void operator++();
		void operator--();
	};
	[[nodiscard]] iterator begin();
	[[nodiscard]] iterator end();
	[[nodiscard]] iterator find(T);
	struct const_iterator : public iterator
	{
		[[nodiscard]] T operator*() = delete;
		[[nodiscard]] const T operator*() const;
	};
	[[nodiscard]] const_iterator begin() const;
	[[nodiscard]] const_iterator end() const;
	[[nodiscard]] const_iterator find(T) const;
	static void init(int maxThreads);
};
