#include "support.h"
#include "../area/area.h"
#include "../geometry/cuboid.h"
#include "../dataStructures/smallSet.h"
#include "../space/space.h"
#include "../items/items.h"

void Support::doStep(Area& area)
{
	Items& items = area.getItems();
	const auto copy = m_maybeFall;
	for(const Cuboid& cuboid : copy)
	{
		const CuboidSet unsupported = getUnsupported(area, cuboid);
		if(!unsupported.empty())
		{
			// A falling chunk is a type of moving platform.
			ItemIndex fallingChunk = area.m_simulation.m_constructedItemTypes.createPlatform(area, unsupported);
			items.maybeFall(fallingChunk);
			m_maybeFall.maybeRemoveAll(unsupported);
		}
		else
			m_maybeFall.maybeRemove(cuboid);
	}
	// Points in maybeFall which have not fallen due to being empty must be cleared.
	m_maybeFall.clear();
}
CuboidSet Support::getUnsupported(const Area& area, const Cuboid& cuboid) const
{
	const Cuboid boundry = area.getSpace().boundry();
	assert(boundry.contains(cuboid));
	const Space& space = area.getSpace();
	CuboidSet openList = space.solid_queryCuboids(cuboid);
	for(const auto& [featureCuboid, pointFeature] : space.pointFeature_getAllWithCuboids(cuboid))
		if(PointFeatureType::byId(pointFeature.pointFeatureType).isSupportAgainstCaveIn)
			openList.maybeAdd(featureCuboid);
	CuboidSet closedList;
	closedList.maybeAddAll(openList);
	while(!openList.empty())
	{
		const Cuboid candidate = openList.back();
		openList.popBack();
		if(candidate.isTouchingFaceFromInside(boundry))
			// If candidate is touching the edge of the space then all are supported.
			return {};
		Cuboid adjacentCuboid = candidate;
		adjacentCuboid.inflate({1});
		for(const Cuboid& adjacentToCandidiate : space.solid_queryCuboids(adjacentCuboid))
			if(candidate != adjacentToCandidiate && candidate.isTouchingFace(adjacentToCandidiate) && !closedList.contains(adjacentToCandidiate))
			{
				closedList.maybeAdd(adjacentToCandidiate);
				openList.maybeAdd(adjacentToCandidiate);
			}
		for(const auto& [featureCuboid, pointFeature] : space.pointFeature_getAllWithCuboids(adjacentCuboid))
			if(candidate != featureCuboid && candidate.isTouchingFace(featureCuboid) && !closedList.contains(featureCuboid))
			{
				if(PointFeatureType::byId(pointFeature.pointFeatureType).isSupportAgainstCaveIn)
					openList.maybeAdd(featureCuboid);
				closedList.maybeAdd(featureCuboid);
			}
	}
	// Group is not supported
	//TODO: check if nonsupporting featureCuboids are supported from elsewhere.
	return closedList;
}
void Support::prepare() { m_support.prepare(); }