#pragma once
#include <array>
#include <cassert>
#include <ranges>
#include "../json.h"

template<typename T, typename Index, int capacity>
struct StrongArray
{
	std::array<T, capacity> data;
	[[nodiscard]] T& operator[](const Index& index) { return data[index.get()]; }
	[[nodiscard]] const T& operator[](const Index& index) const { return data[index.get()]; }
	[[nodiscard]] auto front() -> T& { return data.front(); }
	[[nodiscard]] auto back() -> T& { return data.back(); }
	[[nodiscard]] auto front() const -> const T& { return data.front(); }
	[[nodiscard]] auto back() const -> const T& { return data.back(); }
	[[nodiscard]] auto begin() { return data.begin(); }
	[[nodiscard]] auto end() { return data.end(); }
	[[nodiscard]] const auto begin() const { return data.begin(); }
	[[nodiscard]] const auto end() const { return data.end(); }
	[[nodiscard]] int indexOf(const T& value) const { assert(contains(value)); return std::ranges::find(data, value) - data.begin(); }
	[[nodiscard]] bool contains(const T& value) const { return std::ranges::find(data, value) != data.end(); }
	void fill(const T& value) { data.fill(value); }
};
template<typename T, typename Index, int capacity>
void to_json(Json& data, const StrongArray<T, Index, capacity>& array)
{
	data = Json::array();
	for(const T& value : array.data)
		data.push_back(value);
}
template<typename T, typename Index, int capacity>
void from_json(const Json& data, StrongArray<T, Index, capacity>& array)
{
	Index i{0};
	for(const Json& v : data)
	{
		array[i] = v.get<T>();
		++i;
	}

}