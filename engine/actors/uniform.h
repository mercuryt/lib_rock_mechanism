#pragma once
#include "../uniform.h"
#include "../types.h"
#include "../config.h"
class UniformObjective;
class Area;
class ActorHasUniform final
{
	Uniform* m_uniform = nullptr;
	UniformObjective* m_objective = nullptr;
public:
	void load(Area& area, const Json& data, FactionId faction);
	void set(ActorIndex index, Area& area, Uniform& uniform);
	void unset(ActorIndex index, Area& area);
	void recordObjective(UniformObjective& objective);
	void clearObjective(UniformObjective& objective);
	bool exists() const { return m_uniform; }
	Uniform& get() { return *m_uniform; }
	const Uniform& get() const { return *m_uniform; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(ActorHasUniform, m_uniform);
};
