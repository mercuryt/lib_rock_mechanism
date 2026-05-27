#pragma once
#include "../numericTypes/index.h"

template<typename T>
class WatermarkingStackVector
{
	int m_start;
	static std::vector<std::vector<T>> s_buffer;
	std::vector<T>& get();
public:
	WatermarkingStackVector();
	~WatermarkingStackVector();
	[[ondiscard]] T operator[](int) const;
	[[ondiscard]] T& operator[](int);
	[[nodiscard]] T popAndReturnBack();
	[[nodiscard]] T back();
	[[nodiscard]] T front();
	[[nodiscard]] bool empty() const;
	void push_back(T);
	void pop_back();
	void clear();
	class iterator
	{
		int m_index;
	public:
		[[nodiscard]] T operator*() const;
		[[nodiscard]] T& operator*();
		[[nodiscard]] bool operator==(const iterator& other) const;
		[[nodiscard]] std::strong_ordering operator<=>(const iterator& other) const;
		void operator++();
		void operator--();
	};
	[[nodiscard]] iterator begin();
	[[nodiscard]] iterator end();
	[[nodiscard]] iterator find(T);
	class const_iterator : public iterator
	{
		[[nodiscard]] T operator*() = delete;
		[[nodiscard]] T operator*() const;
	};
	[[nodiscard]] const_iterator begin() const;
	[[nodiscard]] const_iterator end() const;
	[[nodiscard]] const_iterator find(T) const;
	static void init(int maxThreads);
};
