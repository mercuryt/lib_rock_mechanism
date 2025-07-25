#pragma once
#include <concepts>
template <typename T>
concept HasToStringMethod = requires(T a) { a.toString(); };