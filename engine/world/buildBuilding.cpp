#include "buildBuilding.h"
#include "../space/space.h"
#include "../area/area.h"

void buildBuilding::execute(Area& area, Paramaters&& paramaters)
{
	Space& space = area.getSpace();
	CuboidSet shell = paramaters.rooms[0].toSet();
	for(Cuboid cuboid : paramaters.rooms)
		shell.add(cuboid);
	space.solid_setNotAll(shell);
	Cuboid boundry = shell.boundry();
	CuboidSet floorLevel = shell.intersection(boundry.getFaceBelow());
	CuboidSet belowFloorLevel = floorLevel.shiftedDirection(Facing6::Below);
	// Remove sections on floors that will become walls.
	CuboidSet floor = floorLevel.deflated();
	// Make foundation.
	belowFloorLevel.inflateDirection(Facing6::Below);
	space.solid_setAll(belowFloorLevel, paramaters.foundationMaterial, true);
	CuboidSet roof = shell.getFaceAbove();
	CuboidSet interiorWalls;
	CuboidSet exteriorWalls;
	for(Cuboid cuboid : paramaters.rooms)
	{
		CuboidSet walls = cuboid.inflated().toSet();
		walls.remove(cuboid);
		walls.remove(roof);
		walls.remove(floor);
		for(Cuboid wall : walls)
		{
			if(wall.intersects(exteriorWalls))
			{
				CuboidSet wallExteriorParts = wall.toSet();
				// When a wall overlaps an existing exterior wall they both become internal walls.
				if(wall.intersects(wallExteriorParts))
				{
					CuboidSet wallInteriorParts = wall.intersection(exteriorWalls);
					exteriorWalls.remove(wallInteriorParts);
					wallExteriorParts.maybeRemove(wallInteriorParts);
					interiorWalls.maybeAdd(wallInteriorParts);
				}
				exteriorWalls.add(wallExteriorParts);
			}
		}
	}
	for(auto [featureType, cuboids] : paramaters.features)
	{
		exteriorWalls.removeAll(cuboids);
		interiorWalls.removeAll(cuboids);
		roof.removeAll(cuboids);
		floor.removeAll(cuboids);
	}
	space.solid_setAll(exteriorWalls, paramaters.externalWallMaterial, true);
	space.solid_setAll(interiorWalls, paramaters.internalWallMaterial, true);
	if(paramaters.floorMaterial.exists())
		space.pointFeature_construct(floor, PointFeatureTypeId::Floor, paramaters.floorMaterial);
	space.solid_setAll(roof, paramaters.roofMaterial, true);
}
void buildBuilding::mudHut(Area& area, Cuboid location, Facing4 facing)
{
	auto dirt = MaterialType::byName("dirt");
	Point3D doorLocation = location.getFace(facing).intersectionPoint(location.getFaceBelow());
	execute(area, {
		.rooms={location},
		.features={{PointFeatureTypeId::Door, doorLocation.toSet()}},
		.foundationMaterial=dirt,
		.externalWallMaterial=dirt,
		.internalWallMaterial=dirt,
		.roofMaterial=MaterialType::byName("thatch"),
		.floorMaterial=dirt
	});
}