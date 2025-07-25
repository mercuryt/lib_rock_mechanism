#pragma once
#include "point3D.h"
// TODO: Share code with Cuboid?

struct OffsetCuboid
{
	Offset3D m_high;
	Offset3D m_low;
	OffsetCuboid() = default;
	OffsetCuboid(const Offset3D& high, const Offset3D& low);
	OffsetCuboid(const OffsetCuboid& other) = default;
	OffsetCuboid& operator=(const OffsetCuboid& other) = default;
	[[nodiscard]] bool operator==(const OffsetCuboid& other) const = default;
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool contains(const Offset3D& offset) const;
	[[nodiscard]] bool contains(const OffsetCuboid& other) const;
	[[nodiscard]] bool intersects(const OffsetCuboid& other) const;
	[[nodiscard]] bool canMerge(const OffsetCuboid& other) const;
	[[nodiscard]] bool isTouching(const OffsetCuboid& other) const;
	[[nodiscard]] std::pair<Offset3D, Offset3D> toOffsetPair() const { return {m_high, m_low}; }
	[[nodiscard]] SmallSet<OffsetCuboid> getChildrenWhenSplitByCuboid(const OffsetCuboid& cuboid) const;
	void extend(const OffsetCuboid& other);
	class ConstIterator
	{
		OffsetCuboid* m_cuboid;
		Offset3D m_current;
	public:
		ConstIterator() = default;
		ConstIterator(const OffsetCuboid& cuboid, const Offset3D& current);
		ConstIterator& operator=(const ConstIterator& other) = default;
		[[nodiscard]] bool operator==(const ConstIterator& other) const;
		[[nodiscard]] Offset3D operator*() const;
		ConstIterator operator++();
		[[nodiscard]] ConstIterator operator++(int);
	};
	ConstIterator begin() const { return {*this, m_low}; }
	ConstIterator end() const { auto current = m_low; current.z() = m_high.z() + 1; return {*this, current}; }
	[[nodiscard]] Json toJson() const;
	void load(const Json& data);
};
inline void to_json(Json& data, const OffsetCuboid& cuboid) { data = cuboid.toJson(); }
inline void from_json(const Json& data, OffsetCuboid& cuboid) { cuboid.load(data); }