#pragma once
#include "uniform.h"
#include "../types.h"
class Uniform;
class UniformObjective;
class Area;
class ActorHasUniform final
{
	Uniform* m_uniform = nullptr;
	UniformObjective* m_objective = nullptr;
public:
	void set(Uniform& uniform);
	void unset();
	void recordObjective(UniformObjective& objective);
	void clearObjective(UniformObjective& objective);
	bool exists() const { return m_uniform; }
	Uniform& get() { return *m_uniform; }
	const Uniform& get() const { return *m_uniform; }
};
