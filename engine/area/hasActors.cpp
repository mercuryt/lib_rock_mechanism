#include "hasActors.h"
#include "../actor.h"
#include "../block.h"
#include "../area.h"
#include "../simulation.h"
void AreaHasActors::add(Actor& actor)
{
	assert(actor.m_location != nullptr);
	assert(!m_actors.contains(&actor));
	m_actors.insert(&actor);
	m_locationBuckets.insert(actor, *actor.m_location);
	m_visionBuckets.add(actor);
	if(!actor.m_location->m_underground)
		m_onSurface.insert(&actor);
}
void AreaHasActors::remove(Actor& actor)
{
	m_actors.erase(&actor);
	m_locationBuckets.erase(actor);
	m_visionBuckets.remove(actor);
	m_onSurface.erase(&actor);
}
void AreaHasActors::processVisionReadStep()
{
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(m_area.m_simulation.m_step))
		if(actor->m_mustSleep.isAwake())
			m_visionRequestQueue.emplace_back(*actor);
	auto visionIter = m_visionRequestQueue.begin();
	while(visionIter < m_visionRequestQueue.end())
	{
		auto end = m_visionRequestQueue.size() <= (uintptr_t)(m_visionRequestQueue.begin() - visionIter) + Config::visionThreadingBatchSize ?
			m_visionRequestQueue.end() :
			visionIter + Config::visionThreadingBatchSize;
		m_area.m_simulation.m_taskFutures.push_back(m_area.m_simulation.m_pool.submit([=](){ VisionRequest::readSteps(visionIter, end); }));
		visionIter = end;
	}
}
void AreaHasActors::processVisionWriteStep()
{
	for(VisionRequest& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
}
void AreaHasActors::onChangeAmbiantSurfaceTemperature()
{
	for(Actor* actor : m_onSurface)
		actor->m_needsSafeTemperature.onChange();
}
void AreaHasActors::setUnderground(Actor& actor)
{
	m_onSurface.erase(&actor);
	actor.m_needsSafeTemperature.onChange();
}
void AreaHasActors::setNotUnderground(Actor& actor)
{
	m_onSurface.insert(&actor);
	actor.m_needsSafeTemperature.onChange();
}

