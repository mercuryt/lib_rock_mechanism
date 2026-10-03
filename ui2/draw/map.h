#pragma once
#include "../../engine/numericTypes/types.h"
#include "../../engine/geometry/cuboidSet.h"

class Window;

struct DrawMapData
{
	CuboidSet& revealed;
	CuboidSet zSlice;
	Window& window;
	Distance zLevel;
	int offset;
	float darkenFactor;
};

namespace drawMap
{
	void main(Window& window);
	void solidOnCurrentZLevel(DrawMapData& data);
	void drawZLevel(DrawMapData& data);
	void topOfSolid(DrawMapData& data);
	void sidesOfSolid(DrawMapData& data);
	void largeFluidSurface(DrawMapData& data);
	void sidesOfFluid(DrawMapData& data);
	void smallRivers(DrawMapData& data);
	void smallLakes(DrawMapData& data);
	void roads(DrawMapData& data);
	void forests(DrawMapData& data);
}