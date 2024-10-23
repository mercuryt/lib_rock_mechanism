#include "blocks.h"
void Blocks::project_add(const BlockIndex& index, Project& project)
{
	FactionId faction = project.getFaction();
	auto& projects = m_projects[index];
	assert(!projects.contains(faction) || !projects[faction].contains(&project));
	projects.getOrCreate(faction).insert(&project);
}
void Blocks::project_remove(const BlockIndex& index, Project& project)
{
	FactionId faction = project.getFaction();
	auto& projects = m_projects[index];
	assert(projects.contains(faction) && projects[faction].contains(&project));
	if(projects[faction].size() == 1)
		projects.erase(faction);
	else
		projects[faction].erase(&project);
}
Percent Blocks::project_getPercentComplete(const BlockIndex& index, const FactionId& faction) const
{
	auto& projects = m_projects[index];
	if(!projects.contains(faction))
		return Percent::create(0);
	for(Project* project : projects[faction])
		if(project->getPercentComplete().exists())
			return project->getPercentComplete();
	return Percent::create(0);
}
Project* Blocks::project_get(const BlockIndex& index, const FactionId& faction) const
{
	auto& projects = m_projects[index];
	if(!projects.contains(faction))
		return nullptr;
	for(Project* project : projects[faction])
		return project;
	return nullptr;
}
Project* Blocks::project_getIfBegun(const BlockIndex& index, const FactionId& faction) const
{
	auto& projects = m_projects[index];
	if(!projects.contains(faction))
		return nullptr;
	for(Project* project : projects[faction])
		if(project->finishEventExists())
			return project;
	return nullptr;
}
