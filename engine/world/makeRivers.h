#pragma once
#include "../geometry/cuboidSet.h"

struct World;
struct BuildWorld;

struct MakeRivers
{
	CuboidSet m_headwaters;
	std::vector<std::vector<Point3D>> m_rivers;
	World& m_world;
	BuildWorld& m_buildWorld;
	MakeRivers(BuildWorld& buildWorld);
	void makeHeadwaters();
	void makeRivers();
	void recordConnections();
	[[nodiscard]] std::vector<Point3D> pathToRiverOrOcean(Point3D headwaters) const;
};