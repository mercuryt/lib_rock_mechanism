#include "../../src/nthAdjacentOffsets.h"
TEST_CASE("getNthAdjacentOffsets")
{
	auto result = getNthAdjacentOffsets(1);
	CHECK(result.size() == 6);
	CHECK(std::ranges::find(result, XYZ(0,0,1)) != result.end());
	CHECK(std::ranges::find(result, XYZ(0,0,-1)) != result.end());
	CHECK(std::ranges::find(result, XYZ(0,1,0)) != result.end());
	CHECK(std::ranges::find(result, XYZ(0,-1,0)) != result.end());
	CHECK(std::ranges::find(result, XYZ(1,0,0)) != result.end());
	CHECK(std::ranges::find(result, XYZ(-1,0,0)) != result.end());
	CHECK(std::ranges::find(result, XYZ(-2,0,0)) == result.end());
	CHECK(std::ranges::find(result, XYZ(0,0,0)) == result.end());
	CHECK(std::ranges::find(result, XYZ(0,0,2)) == result.end());
	CHECK(std::ranges::find(result, XYZ(0,2,0)) == result.end());
	result = getNthAdjacentOffsets(2);
	CHECK(result.size() == 18);
}
