#include "blocks.h"
void Blocks::project_add(BlockIndex index, Project& project)
{
	Faction* faction = &project.getFaction();
	auto& projects = m_projects.at(index);
	assert(!projects.contains(faction) || !projects.at(faction).contains(&project));
	projects[faction].insert(&project);
}
void Blocks::project_remove(BlockIndex index, Project& project)
{
	Faction* faction = &project.getFaction();
	auto& projects = m_projects.at(index);
	assert(projects.contains(faction) && projects.at(faction).contains(&project));
	if(projects[faction].size() == 1)
		projects.erase(faction);
	else
		projects[faction].erase(&project);
}
Percent Blocks::project_getPercentComplete(BlockIndex index, Faction& faction) const
{
	auto& projects = m_projects.at(index);
	if(!projects.contains(&faction))
		return 0;
	for(Project* project : projects.at(&faction))
		if(project->getPercentComplete())
			return project->getPercentComplete();
	return 0;
}
Project* Blocks::project_get(BlockIndex index, Faction& faction) const
{
	auto& projects = m_projects.at(index);
	if(!projects.contains(&faction))
		return nullptr;
	for(Project* project : projects.at(&faction))
		if(project->finishEventExists())
			return project;
	return nullptr;
}
