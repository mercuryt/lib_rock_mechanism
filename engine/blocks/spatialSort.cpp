#include <variant>
template<typename Zip>
class SpacialSort
{
	bool operator()(Zip a, Zip b) { return mortonNumber(std::get<0>(a)) < mortonNumber(std::get<0>(b)); }
};
