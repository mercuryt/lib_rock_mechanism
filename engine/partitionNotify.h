#pragma once
#include "dataStructures/strongVector.h"

template<typename Func, typename T>
concept ConditionConcept = requires(Func f, const T& value)
{
	{ f(value) } -> std::same_as<bool>;
};

template<typename Func, typename T>
concept CallbackConcept = requires(Func f, const T& oldValue, const T& newValue)
{
	{ f(oldValue, newValue) } -> std::same_as<void>;
};

template<typename T>
concept IterableConcept = std::ranges::bidirectional_range<T>;

// Collect values which fail condition at the start of the vector while calling callback for each move.
// Return first index which passes condition.
template<typename T, typename Index, ConditionConcept<Index> Condition, CallbackConcept<Index> Callback>
Index partitionNotify(StrongVector<T, Index>& target, Condition&& condition, Callback&& callback)
{
	Index forward = Index::create(0);
	Index backward = Index::create(target.size() - 1);
	Index lastDiscarded;
	while(forward != backward)
	{
		if(condition(backward))
		{
			lastDiscarded = backward;
			--backward;
			continue;
		}
		if(!condition(forward))
		{
			++forward;
			continue;
		}
		// Arguments are: oldIndex, newIndex,
		callback(backward, forward);
		target[forward] = std::move(target[backward]);
		lastDiscarded = backward;
		--backward;
		// Don't increment forward here because it may already be equal to backward.
		// Backward decrement must happen now rather then next iteration because move leaves the moved from address in an unknown state.
	}
	return lastDiscarded;
}
// Call partitionNotify and resize the target vector to discard all values which passed the condition.
template<typename T, typename Index, ConditionConcept<Index> Condition, CallbackConcept<Index> Callback>
void removeNotify(StrongVector<T, Index>& target, Condition&& condition, Callback&& callback)
{
	Index index = partitionNotify<T, Index>(target, condition, callback);
	if(index.exists())
		target.resize(index);
}