#include "rectangle.h"
#include "paramaterizedLine.h"
template<Dimensions dimension>
bool Rectangle<dimension>::intercepts(const ParamaterizedLine& line)
{
	/*
	int steps;
	switch(dimension)
	{
		case Dimensions::X:
		{
			auto distance = std::abs((int)m_plane.get() - (int)line.begin.x().get());
			steps = distance / line.sloap[0];
			const auto planeIntercept = line.begin + (steps * line.sloap);
			if(planeIntercept.)
			break;
		}
		case Dimensions::Y:
		{
			auto distance = std::abs((int)m_plane.get() - (int)line.begin.y().get());
			steps = distance / line.sloap[1];
			break;
		}
		case Dimensions::Z:
		{
			auto distance = std::abs((int)m_plane.get() - (int)line.begin.z().get());
			steps = distance / line.sloap[2];
			break;
		}
		default:
			assert(false);
			std::unreachable();
	}
			*/
	return false;

}