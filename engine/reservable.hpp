#include "reservable.h"
template<typename Condition>
SmallMap<Point3D, std::unique_ptr<DishonorCallback>> CanReserve::unreserveAndReturnPointsAndCallbacksWithCondition(Condition&& condition)
{
	SmallMap<Point3D, std::unique_ptr<DishonorCallback>> output;
	auto copy = m_points;
	for(const auto& [point, reservable] : copy)
		if(condition(point))
		{
			std::unique_ptr<DishonorCallback> callback = reservable->unreserveAndReturnCallback(*this);
			output.insert(point, std::move(callback));
		}
	return output;
}