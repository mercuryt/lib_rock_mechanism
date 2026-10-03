#pragma once
#include "smallMap.h"
#include "../numericTypes/types.h"

template<typename T>
struct Schedule
{
	SmallMap<Step, SmallSet<T>> data;
	void insert(Step step, T& data);
	void remove(Step step, T& data);
	void moveTo(Step step, T& data, Schedule<T>& other);
	void putNextStepFirstAndDestroyCurrent();
	[[nodiscard]] std::vector<T>&& getAndEraseStepData(Step step);
};