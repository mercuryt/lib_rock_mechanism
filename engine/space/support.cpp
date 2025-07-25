#include "support.h"
#include "../area/area.h"
#include "../geometry/cuboid.h"
#include "../dataStructures/smallSet.h"
#include "../space/space.h"
#include "../items/items.h"

void Support::doStep(Area& area)
{
	Items& items = area.getItems();
	m_support.prepare();
	while(!m_maybeFall.empty())
	{
		const Cuboid& cuboid = m_maybeFall.front();
		const CuboidSet unsupported = getUnsupported(area, cuboid);
		if(!unsupported.empty())
		{
			// A falling chunk is a type of moving platform.
			ItemIndex fallingChunk = area.m_simulation.m_constructedItemTypes.createPlatform(area, unsupported);
			items.maybeFall(fallingChunk);
			m_maybeFall.removeAll(unsupported);
		}
		else
			m_maybeFall.remove(cuboid);
	}
}
CuboidSet Support::getUnsupported(const Area& area, const Cuboid& cuboid) const
{
	const Cuboid boundry = area.getSpace().boundry();
	assert(boundry.contains(cuboid));
	if(cuboid.isTouchingFaceFromInside(boundry))
		return {};
	SmallSet<Cuboid> openList;
	CuboidSet closedList;
	openList.insert(cuboid);
	closedList.add(cuboid);
	while(!openList.empty())
	{
		openList.sort([&](const Cuboid& a, const Cuboid& b) { return a.m_lowest.z() > b.m_lowest.z(); });
		const Cuboid current = openList.back();
		openList.popBack();
		const auto adjacentToCurrent = m_support.queryGetLeaves(current.inflateAdd(Distance::create(1)));
		for(const Cuboid& adjacent : adjacentToCurrent)
			if(adjacent != current && current.isTouchingFace(adjacent) && !closedList.contains(adjacent))
			{
				if(adjacent.isTouchingFaceFromInside(boundry))
					// Edge found.
					return {};
				closedList.add(adjacent);
				openList.insert(adjacent);
			}
	}
	return closedList;
}