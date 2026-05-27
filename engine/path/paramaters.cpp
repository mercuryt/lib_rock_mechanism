#include "paramaters.h"
#include "../definitions/shape.h"
PathParamaters::PathParamaters(const PathMetaParamaters& params) :
	occupied(params.occupied),
	area(params.area),
	start(params.start),
	huristicDestination(params.huristicDestination),
	shape(params.shape),
	faction(params.faction),
	maxRange(params.maxRange),
	moveType(params.moveType),
	startFacing(params.startFacing),
	depthFirst(params.depthFirst),
	detour(params.detour),
	adjacent(params.adjacent),
	anyOccupiedPoint(params.anyOccupiedPoint),
	returnPath(params.returnPath)
{
	const OffsetCuboid offsetBoundry = Shape::getOffsetCuboidBoundryWithFacing(shape, Facing4::North);
	width = Distance::create(offsetBoundry.sizeX().get());
	length = Distance::create(offsetBoundry.sizeY().get());
	height = Distance::create(offsetBoundry.sizeZ().get());
	singleTile = !Shape::getIsMultiTile(shape);
	assert(depthFirst == huristicDestination.exists());
	assert(start.exists());
	assert(startFacing != Facing4::Null);
	assert(shape.exists());
	assert(moveType.exists());
}