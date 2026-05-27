#include "util.h"
Dimensions util::dimensionForFacing(Facing6 facing)
{
	switch(facing)
	{
		case(Facing6::Above):
		case(Facing6::Below):
			return Dimensions::Z;
		case(Facing6::North):
		case(Facing6::South):
			return Dimensions::Y;
		case(Facing6::East):
		case(Facing6::West):
			return Dimensions::X;
		default:
			std::unreachable();
	}
}