#include "reservable.h"
template<typename Condition>
SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> CanReserve::unreserveAndReturnBlocksAndCallbacksWithCondition(Condition&& condition)
{
	SmallMap<BlockIndex, std::unique_ptr<DishonorCallback>> output;
	auto copy = m_blocks;
	for(const auto& [block, reservable] : copy)
		if(condition(block))
		{
			std::unique_ptr<DishonorCallback> callback = reservable->unreserveAndReturnCallback(*this);
			output.insert(block, std::move(callback));
		}
	return output;
}