/*
	A thin wrapper for rtree boolean to allow space to prevent pathable cuboids from mergeing when a zero thickness partition exists between them.
*/
#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../space/space.h"

struct EnterableData
{
	Distance verticalClearance;
	Temperature maxTemperature = {};
	Temperature minTemperature = {};
	struct Primitive
	{
		DistanceWidth verticalClearance;
		TemperatureWidth maxTemperature;
		TemperatureWidth minTemperature;
		bool operator==(const Primitive& other) const = default;
	};
	[[nodiscard]] EnterableData merge(EnterableData other) const
	{
		return {
			std::min(verticalClearance, other.verticalClearance),
			{},
			{}
			/*
			std::max(maxTemperature, other.maxTemperature),
			std::min(minTemperature, other.minTemperature)
			*/
		};
	}
	[[nodiscard]] constexpr bool operator==(const EnterableData& other) const = default;
	[[nodiscard]] constexpr auto operator<=>(const EnterableData& other) const = default;
	[[nodiscard]] bool empty() const { return verticalClearance.empty(); }
	[[nodiscard]] bool exists() const { return !empty(); }
	[[nodiscard]] std::string toS() const { return "{vertical clearance: " + verticalClearance.toS() + ", maxAndMinTemperature: " + maxTemperature.toS() + "," + minTemperature.toS(); }
	void clear() { verticalClearance.clear(); maxTemperature.clear(); minTemperature.clear(); }
	[[nodiscard]] Primitive get() const { return {verticalClearance.get(), maxTemperature.get(), minTemperature.get()}; }
	static EnterableData create(Primitive data) { return {Distance::create(data.verticalClearance), Temperature::create(data.maxTemperature), Temperature::create(data.minTemperature)}; }
	static constexpr EnterableData null() { return {}; }
	static constexpr Primitive nullPrimitive() { return {.verticalClearance=Distance::nullPrimitive(), .maxTemperature=Temperature::nullPrimitive(), .minTemperature=Temperature::nullPrimitive()}; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(EnterableData, verticalClearance, maxTemperature, minTemperature);
};

class Enterable final : public RTreeData<EnterableData>
{
	const Space& m_space;
	[[nodiscard]] bool canMerge(const Cuboid a, const Cuboid b) const
	{
		return !m_space.move_partitionExistsBetween(a, b);
	}
public:
	Enterable(const Space& space) : RTreeData<EnterableData>(), m_space(space) { }
	void insert(Cuboid cuboid)
	{
		EnterableData data{
			m_space.getVerticalClearance(cuboid)
			// TODO: temperature min and max.
		};
		RTreeData<EnterableData>::insert(cuboid, data);
	}
	void insert(const CuboidSet& cuboids)
	{
		for(Cuboid cuboid : cuboids)
			insert(cuboid);
	}
	// These cuboids must exist as tree leaves with exactly the same location.
	void updateVerticalClearance(const SmallSet<Cuboid>& cuboids)
	{
		//TODO:(optimization) batch getVerticalClearance and updateAction instead of processing cuboids one at a time.
		for(Cuboid cuboid : cuboids)
		{
			Distance clearance = m_space.getVerticalClearance(cuboid);
			constexpr UpdateActionConfig updateConfig{
				.create=false,
				.destroy=false,
				.stopAfterOne=true,
				.allowNotFound=false,
				.allowNotChanged=true
			};
			updateAction<updateConfig>(cuboid, [clearance](EnterableData& data){ data.verticalClearance = clearance; });
		}
	}
};