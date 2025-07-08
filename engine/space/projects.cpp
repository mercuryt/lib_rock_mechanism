#include "space.h"
void Space::project_add(const Point3D& point, Project& project)
{
	FactionId faction = project.getFaction();
	assert(!m_projects.contains(faction) || !m_projects[faction].queryGetOne(point).contains(&project));
	m_projects.getOrCreate(faction).updateOne(point, [&](SmallSet<Project*>& set){ set.insert(&project); });
}
void Space::project_remove(const Point3D& point, Project& project)
{
	FactionId faction = project.getFaction();
	auto& projects = m_projects[faction].queryGetOne(point);
	assert(m_projects.contains(faction) && m_projects[faction].queryGetOne(point).contains(&project));
	if(projects.size() == 1)
		m_projects.erase(faction);
	else
		m_projects[faction].updateOne(point, [&](SmallSet<Project*>& set) { set.erase(&project); });
}
Percent Space::project_getPercentComplete(const Point3D& point, const FactionId& faction) const
{
	if(!m_projects.contains(faction))
		return Percent::create(0);
	auto& projects = m_projects[faction].queryGetOne(point);
	for(Project* project : projects)
		if(project->getPercentComplete().exists())
			return project->getPercentComplete();
	return Percent::create(0);
}
Project* Space::project_get(const Point3D& point, const FactionId& faction) const
{
	if(!m_projects.contains(faction))
		return nullptr;
	auto& projects = m_projects[faction].queryGetOne(point);
	for(Project* project : projects)
		return project;
	return nullptr;
}
Project* Space::project_getIfBegun(const Point3D& point, const FactionId& faction) const
{
	if(!m_projects.contains(faction))
		return nullptr;
	auto& projects = m_projects[faction].queryGetOne(point);
	for(Project* project : projects)
		if(project->finishEventExists())
			return project;
	return nullptr;
}
