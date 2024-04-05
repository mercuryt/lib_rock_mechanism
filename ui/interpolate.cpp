#include "interpolate.h"
float interpolate::interpolate(float from, float to, float delta)
{
	float difference = to - from;
	float offset = difference * delta;
	return from + offset;
}
std::array<float, 2> interpolate::coordinates(std::array<float, 2> from, std::array<float, 2> to, float delta)
{
	return {interpolate(from[0], to[0], delta), interpolate(from[1], to[1], delta)};
}
