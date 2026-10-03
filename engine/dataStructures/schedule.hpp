#include "schedule.h"

template<typename T>
void Schedule<T>::insert(Step step, T& value)
{
	if(data.size() != 1 && data.front().first < step)
	{
		// Replace old front with new step and value, re-add old front to back.
		auto oldFront = std::move(data.front());
		data.front().first = step
		data.front().second.clear();
		data.front().second.insert(value);
		data.insert(oldFront.first, std::move(oldFront.second));
	}
	else
		data.getOrCreate(step).insert(value);
}
template<typename T>
void Schedule<T>::remove(Step step, T& value)
{
	auto found = data.find(step);
	assert(found != data.end());
	if(found->second.size() == 1)
	{
		// This value is the only one for this step. Destroy it rather then leave it empty.
		if(found == data.begin() && data.size() > 1)
		{
			// This is the first pair, and thus the lowest and next step. Find a new first pair.
			putNextStepFirstAndDestroyCurrent();
		}
		else
		{
			// This is not the first pair, destory it without concern to order.
			assert(found->second.front() == value);
			(*found) = data.back();
			data.popBack();
		}
	}
	else
		found->second.erase(value);
}
template<typename T>
void Schedule<T>::moveTo(Step step, T& value, Schedule<T>& other)
{
	remove(step, value);
	other.insert(step, value);
}
template<typename T>
std::vector<T>&& Schedule<T>::getAndEraseStepData(Step step)
{
	if(data.front().first == step)
	{
		auto&& output = std::move(data.front().second);
		putNextStepFirstAndDestroyCurrent();
		return output;
	}
	else
		// No skipping steps.
		assert(data.front().first > step);
}
template<typename T>
void Schedule<T>::putNextStepFirstAndDestroyCurrent()
{
	auto foundNext = std::ranges::min_element(data, {}, [](auto& pair) { return pair.first; });
	data.begin() = (*foundNext);
	(*foundNext) = data.end();
	data.popBack();
}