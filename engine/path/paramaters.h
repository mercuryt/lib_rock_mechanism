#pragma once
#include "../geometry/cuboidSet.h"
#include "../numericTypes/types.h"
#include "../numericTypes/idTypes.h"
class Area;
// The paramaters sent to the constructor for PathParamaters.
struct PathMetaParamaters
{
	CuboidSet occupied = {};
	const Area& area;
	Point3D start;
	Point3D huristicDestination = {};
	ShapeId shape;
	FactionId faction = {};
	Distance maxRange = {};
	MoveTypeId moveType;
	Facing4 startFacing;
	bool depthFirst = false;
	bool detour = false;
	bool adjacent = false;
	bool anyOccupiedPoint = false;
	bool returnPath = true;
};
struct PathParamaters
{
	CuboidSet occupied = {};
	const Area& area;
	Point3D start;
	Point3D huristicDestination = {};
	Distance width;
	Distance length;
	Distance height;
	ShapeId shape;
	FactionId faction = {};
	Distance maxRange = {};
	MoveTypeId moveType;
	Facing4 startFacing;
	bool depthFirst = false;
	bool detour = false;
	bool singleTile = false;
	bool adjacent = false;
	bool anyOccupiedPoint = false;
	bool returnPath = true;
	PathParamaters(const PathMetaParamaters& params);
};