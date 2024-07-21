#pragma once
#include "types.h"
#include "json.h"
struct DishonorCallback
{
	virtual void execute(Quantity oldCount, Quantity newCount) = 0;
	virtual ~DishonorCallback() = default;
	virtual Json toJson() const = 0;
};
