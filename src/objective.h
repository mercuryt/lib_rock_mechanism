#pragma once

class ObjectiveSort
{
	bool operator()(Objective& a, Objective& b){ return a.priority < b.priority; }
};
class Objective
{
	uint32_t m_priority;
	void execute();
	Objective(uint32_t p) : m_priority(p) {}
};
