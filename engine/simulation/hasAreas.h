#pragma once

#include "../area/area.h"
#include "../numericTypes/types.h"
#include "../config/config.h"

#include <string>

class Simulation;
struct DeserializationMemo;

class SimulationHasAreas final
{
	Simulation& m_simulation;
	AreaId m_nextId = AreaId::create(0);
	SmallMap<AreaId, Area*> m_areasById;
	SmallMapStable<AreaId, Area> m_areas;
public:
	SimulationHasAreas(Simulation& simulation) : m_simulation(simulation) { }
	SimulationHasAreas(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation);
	Area& createArea(const Distance& x, const Distance& y, const Distance& z, bool createDrama = false);
	Area& createArea(int32_t x, int32_t y, int32_t z, bool createDrama = false);
	Area& loadArea(const AreaId& id, std::string name, const Distance& x, const Distance& y, const Distance& z);
	Area& loadAreaFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	Area& loadAreaFromPath(const AreaId& id, DeserializationMemo& deserializationMemo);
	void destroyArea(Area& area);
	void loadAreas(const Json& data, DeserializationMemo& deserializationMemo);
	void loadAreas(const Json& data, std::filesystem::path path);
	void doStep();
	void incrementHour();
	void save();
	void clearAll();
	void recordId(Area& area);
	[[nodiscard]] Step getNextStepToSimulate() const;
	[[nodiscard]] Step getNextEventStep() const;
	[[nodiscard]] Area& getById(const AreaId& id) const {return *m_areasById[id]; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] SmallMapStable<AreaId, Area>& getAll() { return m_areas; }
};
