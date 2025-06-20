#pragma once
#include "numericTypes/types.h"
#include "json.h"
struct DishonorCallback
{
	virtual void execute(const Quantity& oldCount, const Quantity& newCount) = 0;
	virtual ~DishonorCallback() = default;
	virtual Json toJson() const = 0;
};
