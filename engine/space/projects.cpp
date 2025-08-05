#include "space.h"

void Space::project_add(const Point3D& point, Project& project)
{
	m_projects.getOrCreate(project.getFaction()).insert(point, RTreeDataWrapper<Project*, nullptr>(&project));
}
void Space::project_remove(const Point3D& point, Project& project)
{
	m_projects[project.getFaction()].remove(point, RTreeDataWrapper<Project*, nullptr>(&project));
}
Percent Space::project_getPercentComplete(const Point3D& point, const FactionId& faction) const
{
	const auto found = m_projects.find(faction);
	if(found == m_projects.end())
		return Percent::create(0);
	auto& projects = found.second();
	for(const RTreeDataWrapper<Project*, nullptr>& project : projects.queryGetAll(point))
	{
		Percent output = project.get()->getPercentComplete();
		if(output.empty())
			continue;
		return output;
	}
	return {0};
}
Project* Space::project_get(const Point3D& point, const FactionId& faction) const
{
	const auto found = m_projects.find(faction);
	if(found == m_projects.end())
		return nullptr;
	auto& projects = found.second();
	return projects.queryGetFirst(point).get();
}
Project* Space::project_getIfBegun(const Point3D& point, const FactionId& faction) const
{
	const auto found = m_projects.find(faction);
	if(found == m_projects.end())
		return nullptr;
	auto& projects = found.second();
	const auto condition = [&](const RTreeDataWrapper<Project*, nullptr>& project){ return project.get()->finishEventExists(); };
	return projects.queryGetOneWithCondition(point, condition).get();
}