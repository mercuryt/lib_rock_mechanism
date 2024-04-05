#include <array>
namespace interpolate
{
	float interpolate(float from, float to, float delta);
	std::array<float, 2> coordinates(std::array<float, 2> from, std::array<float, 2> to, float delta);
}
