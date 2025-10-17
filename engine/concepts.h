#pragma once
#include <concepts>
#include "json.h"
template <typename T>
concept HasToStringMethod = requires(T a) { a.toString(); };

template <typename T>
concept HasHashMethod = requires(T::Hash h) { h(std::declval<T>()); };

template <typename T>
concept HasGetMethod = requires(T a) { a.get(); };

template <typename T>
concept HasEmptyMethod = requires(T a) { a.empty(); };

template <typename T>
concept HasToJsonMethod = requires(T a, Json json) { to_json(json, a); };

template <typename T>
concept HasFromJsonMethod = requires(T a, Json json) { from_json(json, a); };

template<typename T>
concept MoveConstructible = std::is_move_constructible_v<T>;

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept Sortable = requires(T a, T b) {
	{ a < b } -> std::same_as<bool>; // Check if operator< exists and returns bool
};