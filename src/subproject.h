#pragma once
class Project;
class Subproject
{
	Project& m_project;
public:
	std::unordered_set<Actor*> m_workers;
	Subproject(Project& p) : m_project(p) { }
	virtual void commandWorker(Actor& actor) = 0;
	virtual void onComplete() = 0;
};
