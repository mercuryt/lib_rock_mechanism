#pragma once

#include <unordered_set>

template<class Project>
class ProjectFinishEvent : public ScheduledEventWithPercent
{
	Project& m_project;
	ProjectFinishEvent(uint32_t step, Project& p) : ScheduledEventWithPercent(step), m_project(p) {}
	void execute() { m_project.complete(); }
	~ProjectFinishEvent() { m_project.m_finishEvent.clearPointer(); }
};

// Derived classes are expected to provide getDelay and onComplete.
template<class Actor>
class Project
{
	HasScheduledEventPausable<ProjectFinishEvent> m_finishEvent;
protected:
	std::unordered_set<Actor*> m_workers;
public:
	void addWorker(Actor& actor)
	{
		assert(actor.m_project == nullptr);
		assert(!m_workers.contains(&actor):
		actor.m_project = this;
		m_workers.insert(&actor);
		scheduleEvent();
	}
	void removeWorker(Actor& actor)
	{
		assert(actor.m_project == this);
		assert(m_workers.contains(&actor):
		actor.m_project = nullptr;
		m_workers.remove(&actor);
		scheduleEvent();
	}
	void complete()
	{
		dismissWorkers();
		onComplete();
	}
	void cancel()
	{
		m_finishEvent.maybeCancel();
		dismissWorkers();
	}
	void dismissWorkers()
	{
		for(Actor* actor : m_workers)
			actor->taskComplete();
	}
	void scheduleEvent()
	{
		m_finishEvent.maybeCancel();
		uint32_t delay = util::scaleByPercent(getDelay(), 100u - m_digEvent.percentComplete());
		m_finishEvent.schedule(::s_step + delay, *this);
	}
	virtual ~Project
	{
		dismissWorkers();
	}
	virtual uint32_t getDelay() const;
	virtual void onComplete();
};
