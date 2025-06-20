#pragma once
#include "numericTypes/index.h"
#include "numericTypes/types.h"

template<typename Func>
concept DestinationCondition = requires(Func f, const BlockIndex& b, const Facing4& fa)
{
	{ f(b, fa) } -> std::same_as<std::pair<bool, BlockIndex>>;
};
template<typename Func>
concept AccessCondition = requires(Func f, const BlockIndex& b, const Facing4& fa)
{
	{ f(b, fa) } -> std::same_as<bool>;
};