#include "map.h"
#include "../window.h"
#include "../sprite.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/world/world.h"
void drawMap::main(Window& window, CuboidSet& revealed)
{
	Distance minValueOfMap = window.m_simulation->m_world->m_boundry.m_low.z();
	Distance lowestDrawDepth = minValueOfMap + window.m_drawDepth > window.m_pov->z ?
		minValueOfMap : window.m_pov->z - window.m_drawDepth;
	CuboidSet revealedZSlice;
	DrawMapData data{revealed, revealedZSlice, window, lowestDrawDepth};
	Distance end = window.m_pov->z;
	World& world = *window.m_simulation->m_world;
	while(data.zLevel <= end)
	{
		Distance depth = window.m_pov->z - data.zLevel;
		data.offset = depth.get() * displayData::offsetToTheSouthWestInPixelsPerZLevelDrawDistance;
		data.darkenFactor = (float)(depth.get() + 1) / (float)window.m_drawDepth;
		data.zSlice = revealed.intersection(world.m_boundry.slicedAtZ(data.zLevel));
		drawZLevel(data);
		++data.zLevel;
	}
	solidOnCurrentZLevel(data);
}
void drawMap::drawZLevel(DrawMapData& data)
{
	topOfSolid(data);
	sidesOfSolid(data);
	largeFluidSurface(data);
	sidesOfFluid(data);
	smallRivers(data);
	smallLakes(data);
	forests(data);
	roads(data);
}
void drawMap::topOfSolid(DrawMapData& data)
{
	World& world = *data.window.m_simulation->m_world;
	// Nothing to display of on the lowest level.
	if(data.zLevel == world.m_boundry.m_low.z())
		return;
	// TODO: display different grassland and desert terrain.
	static auto roughFloorSprite = Sprite("roughFloor");
	static SDL_Color dirtColor = displayData::materialColors[MaterialType::byName("dirt")];
	SDL_Color darkenedDirt = displayData::darkenColor(dirtColor, data.darkenFactor);
	Distance depth = data.window.m_pov->z - data.zLevel;
	CuboidSet query = data.revealed.intersection(world.m_boundry.slicedAtZ(data.zLevel - 1));
	CuboidSet result = world.m_solid.queryGetAllCuboidsWithCondition(query, [&data](Cuboid cuboid){ return cuboid.m_high.z() == data.zLevel - 1; });
	for(Cuboid cuboid : result)
		roughFloorSprite.drawRepeatedAndTintedAndOffsetSouthWest(data.window, cuboid, darkenedDirt, data.offset);
}
void drawMap::sidesOfSolid(DrawMapData& data)
{
	static auto roughWallCorner = Sprite("roughWallCorner");
	static auto roughWallSouthSprite = Sprite("roughWallSouth");
	static auto roughWallWestSprite = Sprite("roughWallWest");
	static SDL_Color dirtColor = displayData::materialColors[MaterialType::byName("dirt")];
	World& world = *data.window.m_simulation->m_world;
	SDL_Color darkenedDirt = displayData::darkenColor(dirtColor, data.darkenFactor);
	for(Cuboid cuboid : world.m_solid.queryGetAllCuboids(data.revealed.intersection(world.m_boundry.slicedAtZ(data.zLevel))))
	{
		cuboid = cuboid.slicedAtZ(data.zLevel);
		if(cuboid.m_low.x() != world.m_boundry.m_low.x() && cuboid.m_low.y() != world.m_boundry.m_low.y())
		{
			// Corner
			Point3D location = cuboid.m_low;
			location = Point3D::create(location.applyOffset(Offset3D(-1, -1, 0)));
			roughWallCorner.drawTintedAndRightAlignedAndOffsetSouthWest(data.window, location, darkenedDirt, data.offset);
		}
		if(cuboid.m_low.x() != world.m_boundry.m_low.x())
		{
			// West wall
			Cuboid edge = cuboid.getFaceWest();
			edge.shift(Facing6::West);
			roughWallWestSprite.drawRepeatedVerticallyAndTintedAndRightAlignedAndOffsetSouthWest(data.window, edge, darkenedDirt, data.offset);
		}
		if(cuboid.m_low.y() != world.m_boundry.m_low.y())
		{
			// South wall
			Cuboid edge = cuboid.getFaceSouth();
			edge.shift(Facing6::South);
			roughWallSouthSprite.drawRepeatedVerticallyAndTintedAndRightAlignedAndOffsetSouthWest(data.window, edge, darkenedDirt, data.offset);
		}
	}
}
void drawMap::largeFluidSurface(DrawMapData& data)
{
	static auto fluidSprite = Sprite("fluidSurface");
	World& world = *data.window.m_simulation->m_world;
	// Nothing to display of on the lowest level.
	if(data.zLevel == world.m_boundry.m_low.z())
		return;
	Distance depth = data.window.m_pov->z - data.zLevel;
	CuboidSet query = data.revealed.intersection(world.m_boundry.slicedAtZ(data.zLevel - 1));
	for(auto [cuboid, fluidType] : world.m_fluid.queryGetAllWithCuboidsAndCondition(
		query,
		[&data](Cuboid cuboid){ return cuboid.m_high.z() == data.zLevel - 1; }
	))
	{
		SDL_Color fluidColor = displayData::fluidColors[fluidType];
		SDL_Color darkenedColor = displayData::darkenColor(fluidColor, data.darkenFactor);
		fluidSprite.drawRepeatedAndTintedAndOffsetSouthWest(data.window, cuboid, darkenedColor, data.offset);
	}
}
void drawMap::sidesOfFluid(DrawMapData& data)
{
	static auto cornerSprite = Sprite("roughWallCorner");
	static auto southSprite = Sprite("roughWallSouth");
	static auto westSprite = Sprite("roughWallWest");
	World& world = *data.window.m_simulation->m_world;
	for(auto [cuboid, fluidType] : world.m_fluid.queryGetAllWithCuboids(data.revealed.intersection(world.m_boundry.slicedAtZ(data.zLevel))))
	{
		cuboid = cuboid.slicedAtZ(data.zLevel);
		if(cuboid.m_low.x() != world.m_boundry.m_low.x() && cuboid.m_low.y() != world.m_boundry.m_low.y())
		{
			// Corner
			Point3D location = cuboid.m_low;
			location = Point3D::create(location.applyOffset(Offset3D(-1, -1, 0)));
			SDL_Color color = displayData::fluidColors[fluidType];
			SDL_Color darkenedColor = displayData::darkenColor(color, data.darkenFactor);
			cornerSprite.drawTintedAndRightAlignedAndOffsetSouthWest(data.window, location, darkenedColor, data.offset);
		}
		if(cuboid.m_low.x() != world.m_boundry.m_low.x())
		{
			// West wall
			Cuboid edge = cuboid.getFaceWest();
			edge.shift(Facing6::West);
			SDL_Color color = displayData::fluidColors[fluidType];
			SDL_Color darkenedColor = displayData::darkenColor(color, data.darkenFactor);
			westSprite.drawRepeatedVerticallyAndTintedAndRightAlignedAndOffsetSouthWest(data.window, edge, darkenedColor, data.offset);
		}
		if(cuboid.m_low.y() != world.m_boundry.m_low.y())
		{
			// South wall
			Cuboid edge = cuboid.getFaceSouth();
			edge.shift(Facing6::South);
			SDL_Color color = displayData::fluidColors[fluidType];
			SDL_Color darkenedColor = displayData::darkenColor(color, data.darkenFactor);
			southSprite.drawRepeatedVerticallyAndTintedAndRightAlignedAndOffsetSouthWest(data.window, edge, darkenedColor, data.offset);
		}
	}
}
void drawMap::smallRivers(DrawMapData& data)
{
	World& world = *data.window.m_simulation->m_world;
	SDL_Color fluidColor = displayData::fluidColors[world.m_oceanFluidType];
	SDL_Color darkenedColor = displayData::darkenColor(fluidColor, data.darkenFactor);
	for(auto [cuboid, smallRiverData] : world.m_smallRivers.queryGetAllWithCuboids(data.zSlice))
	{
		int thickness = smallRiverData.flowRate / displayData::unitsOfFlowRatePerRiverDisplayThickness;
		for(Point3D point : cuboid)
		{
			SDL_Rect destination{
				point.x().get() * displayData::defaultScale,
				point.y().get() * displayData::defaultScale,
				displayData::defaultScale,
				displayData::defaultScale
			};
			data.window.m_renderBuffer.addNetwork(destination, darkenedColor, thickness, smallRiverData.connections);
		}
	}
}
void drawMap::roads(DrawMapData& data)
{
	World& world = *data.window.m_simulation->m_world;
	SDL_Color darkenedColor = displayData::darkenColor(displayData::roadColor, data.darkenFactor);
	for(auto [cuboid, connections] : world.m_roads.queryGetAllWithCuboids(data.zSlice))
		for(Point3D point : cuboid)
		{
			SDL_Rect destination{
				point.x().get() * displayData::defaultScale,
				point.y().get() * displayData::defaultScale,
				displayData::defaultScale,
				displayData::defaultScale
			};
			data.window.m_renderBuffer.addNetwork(destination, darkenedColor, displayData::roadThickness, connections);
		}
}
void drawMap::smallLakes(DrawMapData& data)
{
	World& world = *data.window.m_simulation->m_world;
	static auto lakesSprite = Sprite("lakes");
	SDL_Color fluidColor = displayData::fluidColors[world.m_oceanFluidType];
	SDL_Color darkenedColor = displayData::darkenColor(fluidColor, data.darkenFactor);
	for(Cuboid cuboid : world.m_smallLakes.queryGetLeaves(data.zSlice))
		lakesSprite.drawRepeatedAndTintedAndOffsetSouthWest(data.window, cuboid, darkenedColor, data.offset);
}
void drawMap::forests(DrawMapData& data)
{
	World& world = *data.window.m_simulation->m_world;
	static auto treeSprite = Sprite("tree1");
	for(auto [cuboid, treeCount] : world.m_trees.queryGetAllWithCuboids(data.zSlice))
	{
		float scale = (float)treeCount.get() / displayData::minimumNumberOfTreesToDisplayOnMapAs100Percent;
		treeSprite.drawRepeatedAndScaledAndOffsetSouthWest(data.window, cuboid.toSet(), scale, data.offset);
	}
}