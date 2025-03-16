#include "uniform.h"
#include "actors.h"
#include "../objective.h"
#include "../uniform.h"
#include "../objectives/uniform.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
void ActorHasUniform::load(Area& area, const Json& data, const FactionId& faction)
{
	if(data.contains("m_uniform"))
		m_uniform = &area.m_simulation.m_hasUniforms.getForFaction(faction).byName(data["m_uniform"].get<std::string>());
	// Don't serialize uniform objective, create from the objective deserialization instead.
}
void ActorHasUniform::set(const ActorIndex& index, Area& area, Uniform& uniform)
{
	assert(m_uniform == nullptr);
	if(m_objective)
		m_objective->cancel(area, index);
	m_uniform = &uniform;
}
void ActorHasUniform::unset(const ActorIndex& index, Area& area)
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
void ActorHasUniform::clearObjective([[maybe_unused]] UniformObjective& objective)
{
	assert(m_objective);
	assert(*m_objective == objective);
	m_objective = nullptr;
}
void Actors::uniform_set(const ActorIndex& index, Uniform& uniform)
{
	if(m_hasUniform[index] == nullptr)
		m_hasUniform[index] = std::make_unique<ActorHasUniform>();
	else
		m_hasUniform[index]->unset(index, m_area);
	m_hasUniform[index]->set(index, m_area, uniform);
	std::unique_ptr<Objective> objective = std::make_unique<UniformObjective>(m_area, index);
	m_hasUniform[index]->recordObjective(*static_cast<UniformObjective*>(objective.get()));
	m_hasObjectives[index]->addTaskToStart(m_area, std::move(objective));
}
void Actors::uniform_unset(const ActorIndex& index)
{
	assert(uniform_exists(index));
	m_hasUniform[index]->unset(index, m_area);
	m_hasUniform[index] = nullptr;
}
bool Actors::uniform_exists(const ActorIndex& index) const
{
	return m_hasUniform[index] != nullptr;
}
Uniform& Actors::uniform_get(const ActorIndex& index)
{
	return m_hasUniform[index]->get();
}
const Uniform& Actors::uniform_get(const ActorIndex& index) const
{
	return m_hasUniform[index]->get();
}
