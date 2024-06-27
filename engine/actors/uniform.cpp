#include "uniform.h"
#include "actors.h"
#include "../objective.h"
#include "../uniform.h"
void ActorHasUniform::set(Uniform& uniform)
{
	if(m_objective)
		m_objective->cancel();
	m_uniform = &uniform;
}
void ActorHasUniform::unset()
{
	if(m_objective)
		m_objective->cancel();
	m_uniform = nullptr;
}
void ActorHasUniform::recordObjective(UniformObjective& objective)
{
	assert(!m_objective);
	m_objective = &objective;
}
void ActorHasUniform::clearObjective([[maybe_unused]] UniformObjective& objective)
{
	assert(m_objective);
	assert(*m_objective == objective);
	m_objective = nullptr;
}
void Actors::uniform_set(ActorIndex index, Uniform& uniform)
{
	if(uniform_exists(index))
		uniform_unset(index);
	m_hasUniform.at(index).set(uniform);
	std::unique_ptr<Objective> objective = std::make_unique<UniformObjective>(m_area, index);
	m_hasUniform.at(index).recordObjective(*static_cast<UniformObjective*>(objective.get()));
	m_hasObjectives.at(index).addTaskToStart(std::move(objective));
}
void Actors::uniform_unset(ActorIndex index)
{
	assert(uniform_exists(index));
	m_hasUniform.at(index).unset();
	m_hasUniform.erase(index);
}
bool Actors::uniform_exists(ActorIndex index) const
{
	return m_hasUniform.contains(index);
}
Uniform& Actors::uniform_get(ActorIndex index)
{
	return m_hasUniform.at(index).get();
}
const Uniform& Actors::uniform_get(ActorIndex index) const
{
	return m_hasUniform.at(index).get();
}
