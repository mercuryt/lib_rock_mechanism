#pragma once

#include "../area.h"
#include "../types.h"
#include "../config.h"

#include <string>

class Simulation;
struct DeserializationMemo;

class SimulationHasAreas final
{
	Simulation& m_simulation;
	AreaId m_nextId = 0;
	std::unordered_map<AreaId, Area*> m_areasById;
	std::list<Area> m_areas;
public:
	SimulationHasAreas(Simulation& simulation) : m_simulation(simulation) { }
	SimulationHasAreas(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation);
	Area& createArea(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z, bool createDrama = false);
	Area& loadArea(AreaId id, std::wstring name, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z);
	Area& loadAreaFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	Area& loadAreaFromPath(AreaId id, DeserializationMemo& deserializationMemo);
	void destroyArea(Area& area);
	void loadAreas(const Json& data, DeserializationMemo& deserializationMemo);
	void loadAreas(const Json& data, std::filesystem::path path);
	void readStep();
	void writeStep();
	void incrementHour();
	void save();
	void clearAll();
	void recordId(Area& area);
	[[nodiscard]] Area& getById(AreaId id) const {return *m_areasById.at(id); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::list<Area>& getAll() { return m_areas; }
};
