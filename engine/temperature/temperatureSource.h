#pragma once
#include "../geometry/cuboidSet.h"
#include "../numericTypes/types.h"
#include "../numericTypes/idTypes.h"
#include "../dataStructures/rtreeData.h"
#include "../json.h"
class Area;
struct TemperatureSource2
{
	struct Primitive
	{
		Point3D::Primitive location;
		TemperatureSourceIdWidth id;
		TemperatureDeltaWidth delta;
		bool operator<=>(const Primitive& other) const = default;
		bool operator==(const Primitive& other) const { return other.id == id; }
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Primitive, location, id, delta);
	};
	Point3D m_location;
	TemperatureSourceId m_id;
	TemperatureDelta m_delta;
	void updateDelta(Area& area, TemperatureDelta newValue);
	void clear() { m_location.clear(); m_id.clear(); m_delta.clear(); }
	[[nodiscard]] std::strong_ordering operator<=>(const TemperatureSource2& other) const = default;
	[[nodiscard]] Primitive get() const { return {m_location.get(), m_id.get(), m_delta.get()}; }
	[[nodiscard]] constexpr static TemperatureSource2 null() { return {}; }
	[[nodiscard]] constexpr static Primitive nullPrimitive() { return {Point3D::nullPrimitive(), TemperatureSourceId::nullPrimitive(), TemperatureDelta::nullPrimitive()}; }
	[[nodiscard]] constexpr static TemperatureSource2 create(const Primitive& primitive) { return {Point3D::create(primitive.location), TemperatureSourceId::create(primitive.id), TemperatureDelta::create(primitive.delta)}; }
	[[nodiscard]] constexpr static TemperatureSource2 create(Point3D location, TemperatureSourceId id, TemperatureDelta delta) { return {location, id, delta}; }
	[[nodiscard]] std::string toString() const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(TemperatureSource2, m_location, m_id, m_delta);
};
class AreaHasTemperatureSources
{
	RTreeData<TemperatureSource2, RTreeDataConfigs::canOverlapNoMerge> m_data;
	std::vector<std::pair<Point3D, TemperatureSourceId>> m_sourcesToUpdate;
	TemperatureSourceId m_nextId{0};
	std::vector<TemperatureSourceId> m_unusedIds;
public:
	TemperatureSourceId addTemperatureSource(Area& area, const Point3D location, const TemperatureDelta delta);
	void removeTemperatureSource(Area& area, const Point3D location, const TemperatureSourceId id);
	void updateTemperatureSourceDelta(Area& area, const Point3D location, const TemperatureDelta oldDelta, const TemperatureSourceId id, const TemperatureDelta newDelta);
	// Returns area which is exposed to sky but also in range of at least one temperature source. Exclude this area from normal ambient temperature change. It has already been marked for processing in the context of this object.
	[[nodiscard]] CuboidSet onChangeAmbiantSurfaceTemperatureReturnIntersection(Area& area);
	void doStep(Area& area);
	void releaseId(TemperatureSourceId id);
	void onTemperatureCanNoLongerTransmit(const CuboidSet& cuboids);
	void onTemperatureCanNowTransmit(const CuboidSet& cuboids);
	[[nodiscard]] TemperatureDelta getDelta(const Point3D point);
	[[nodiscard]] TemperatureSourceId getNextId();
	[[nodiscard]] CuboidSet getPointsIntersectingExposedToSky(Area& area) const;
	[[nodiscard]] static CuboidSet getAffectedArea(Area& area, const Point3D location, const TemperatureDelta delta);
	GDB_CALLABLE std::string toString(Area& area, int x, int y, int z);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasTemperatureSources, m_data, m_sourcesToUpdate, m_nextId, m_unusedIds);
};