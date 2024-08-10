#include "uniform.h"
#include "actors.h"
#include "../objective.h"
#include "../uniform.h"
#include "../objectives/uniform.h"
void ActorHasUniform::set(ActorIndex index, Area& area, Uniform& uniform)
{
	if(m_objective)
		m_objective->cancel(area, index);
	m_uniform = &uniform;
}
void ActorHasUniform::unset(ActorIndex index, Area& area)
{
	if(m_objective)
		m_objective->cancel(area, index);
	m_uniform = nullptr;
}
void ActorHasUniform::recordObjective(UniformObjective& objective)
{
	assert(!m_objective);
	m_objective = &objective;
}
void ActorHasUniform::clearObjective(UniformObjective& objective)
{
	assert(m_objective);
	assert(*m_objective == objective);
	m_objective = nullptr;
}
void Actors::uniform_set(ActorIndex index, Uniform& uniform)
{
	if(uniform_exists(index))
		uniform_unset(index);
	m_hasUniform[index]->set(index, m_area, uniform);
	std::unique_ptr<Objective> objective = std::make_unique<UniformObjective>(m_area, index);
	m_hasUniform[index]->recordObjective(*static_cast<UniformObjective*>(objective.get()));
	m_hasObjectives[index]->addTaskToStart(m_area, std::move(objective));
}
void Actors::uniform_unset(ActorIndex index)
{
	assert(uniform_exists(index));
	m_hasUniform[index]->unset(index, m_area);
	m_hasUniform[index] = nullptr;
}
bool Actors::uniform_exists(ActorIndex index) const
{
	return m_hasUniform[index] != nullptr;
}
Uniform& Actors::uniform_get(ActorIndex index)
{
	return m_hasUniform[index]->get();
}
const Uniform& Actors::uniform_get(ActorIndex index) const
{
	return m_hasUniform[index]->get();
}
