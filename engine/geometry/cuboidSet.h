#pragma once
#include "cuboid.h"
#include "offsetCuboid.h"
#include "../numericTypes/index.h"
#include "../dataStructures/smallSet.h"

struct OffsetCuboidSet;
template<typename CuboidType, typename PointType, typename CuboidSetType>
struct CuboidSetConstIteratorBase
{
	const CuboidSetType& m_cuboidSet;
	SmallSet<CuboidType>::const_iterator m_outerIter;
	CuboidType::ConstIterator m_innerIter;
public:
	CuboidSetConstIteratorBase(const CuboidSetType& cuboidSet, bool end = false);
	CuboidSetConstIteratorBase& operator++();
	[[nodiscard]] CuboidSetConstIteratorBase operator++(int);
	[[nodiscard]] bool operator==(const CuboidSetConstIteratorBase& other) const { return m_outerIter == other.m_outerIter && m_innerIter == other.m_innerIter; }
	[[nodiscard]] bool operator!=(const CuboidSetConstIteratorBase& other) const { return !(*this == other); }
	[[nodiscard]] PointType operator*() const { assert(m_innerIter != m_outerIter->end()); return *m_innerIter; }
};
template<typename CuboidType, typename PointType, typename CuboidSetType>
struct CuboidSetBase
{
protected:
	// Overriden by AreaHasVisionCuboids.
	virtual void insertOrMerge(const CuboidType& cuboid);
	virtual void destroy(const uint& cuboid);
	// For merging contained cuboids.
	void mergeInternal(const CuboidType& absorbed, const uint& absorber);
public:
	SmallSet<CuboidType> m_cuboids;
	CuboidSetBase() = default;
	CuboidSetBase(const CuboidSetType& other) : m_cuboids(other.m_cuboids) { }
	CuboidSetBase(CuboidSetType&& other) noexcept : m_cuboids(std::move(other.m_cuboids)) { }
	CuboidSetBase(const PointType& location, const Facing4& rotation, const OffsetCuboidSet& offsetPairs);
	CuboidSetBase(const std::initializer_list<CuboidType>& cuboids);
	CuboidSetBase(const CuboidType& cuboid) : m_cuboids({cuboid}) { }
	CuboidSetBase(const SmallSet<CuboidType>& cuboids) : m_cuboids(cuboids) { }
	CuboidSetBase(const SmallSet<PointType>& points) { for(const PointType& point : points) add(point); }
	CuboidSetType& operator=(CuboidSetType&& other) noexcept { m_cuboids = std::move(other.m_cuboids); return static_cast<CuboidSetType&>(*this); }
	CuboidSetType& operator=(const CuboidSetType& other) { m_cuboids = other.m_cuboids; return static_cast<CuboidSetType&>(*this); }
	void maybeAdd(const PointType& point);
	void maybeAddAll(const CuboidSetType& other);
	void add(const auto& shape)
	{
		assert(!shape.empty());
		assert(!intersects(shape));
		maybeAdd(shape);
	}
	void addAll(const auto& shape)
	{
		assert(!shape.empty());
		assert(!intersects(shape));
		maybeAddAll(shape);
	}
	void maybeRemove(const PointType& point);
	void maybeRemoveAll(const auto& cuboids) { for(const CuboidType& cuboid : cuboids) maybeRemove(cuboid); }
	void remove(const auto& shape)
	{
		assert(!shape.empty());
		assert(intersects(shape));
		maybeRemove(shape);
	}
	void removeAll(const auto& shape)
	{
		assert(!shape.empty());
		assert(intersects(shape));
		maybeRemoveAll(shape);
	}
	// TODO: why virtual?
	virtual void maybeAdd(const CuboidType& cuboid);
	virtual void maybeRemove(const CuboidType& cuboid);
	void clear() { m_cuboids.clear(); }
	void shift(const Offset3D offset, const Distance& distance);
	// For merging with other cuboid sets.
	void addSet(const CuboidSetType& other);
	void rotateAroundPoint(const PointType& point, const Facing4& rotation);
	void reserve(const uint16_t& capacity) { m_cuboids.reserve(capacity); }
	void swap(CuboidSetType& other);
	void popBack();
	void inflate(const Distance& distance);
	[[nodiscard]] const CuboidType& operator[](const uint8_t& index) const { return m_cuboids[index]; }
	[[nodiscard]] CuboidType& operator[](const uint8_t& index){ return m_cuboids[index]; }
	[[nodiscard]] PointType center() const;
	[[nodiscard]] PointType::DimensionType lowestZ() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool exists() const;
	[[nodiscard]] uint size() const;
	[[nodiscard]] uint volume() const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Offset3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] bool contains(const OffsetCuboid& cuboid) const;
	[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
	[[nodiscard]] SmallSet<PointType> toPointSet() const;
	[[nodiscard]] bool isAdjacent(const PointType& cuboid) const;
	[[nodiscard]] bool isAdjacent(const CuboidType& cuboid) const;
	[[nodiscard]] CuboidType boundry() const;
	[[nodiscard]] PointType getLowest() const;
	[[nodiscard]] const CuboidType& front() const { return m_cuboids.front(); }
	[[nodiscard]] const CuboidType& back() const { return m_cuboids.back(); }
	[[nodiscard]] auto begin() { return m_cuboids.begin(); }
	[[nodiscard]] auto end() { return m_cuboids.end(); }
	[[nodiscard]] auto begin() const { return m_cuboids.begin(); }
	[[nodiscard]] auto end() const { return m_cuboids.end(); }
	[[nodiscard]] CuboidSetType intersection(const CuboidType& cuboid) const;
	[[nodiscard]] CuboidSetType intersection(const CuboidSetType& cuboid) const;
	[[nodiscard]] PointType intersectionPoint(const CuboidType& cuboid) const;
	[[nodiscard]] bool intersects(const PointType& point) const;
	[[nodiscard]] bool intersects(const CuboidType& cuboid) const;
	[[nodiscard]] bool intersects(const CuboidSetType& cuboid) const;
	[[nodiscard]] bool isTouching(const CuboidType& cuboid) const;
	[[nodiscard]] bool isTouchingFace(const CuboidType& cuboid) const;
	[[nodiscard]] bool isTouchingFaceFromInside(const CuboidType& cuboid) const;
	[[nodiscard]] bool isTouching(const CuboidSetType& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(const CuboidSetType& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(const CuboidType& cuboid) const;
	[[nodiscard]] CuboidSetType getAdjacent() const;
	[[nodiscard]] CuboidSetType getDirectlyAdjacent(const Distance& distance) const;
	[[nodiscard]] CuboidSetType inflateFaces(const Distance& distance) const;
	[[nodiscard]] uint countIf(auto&& condition) const
	{
		uint output = 0;
		for(const Cuboid& cuboid : m_cuboids)
			output += cuboid.countIf(condition);
		return output;
	}
	[[nodiscard]] __attribute__((noinline)) std::string toString() const;
	[[nodiscard]] static CuboidSetType create(const SmallSet<PointType>& space);
	[[nodiscard]] static CuboidSetType create(const CuboidSetType& set);
	[[nodiscard]] static CuboidSetType create(const CuboidType& cuboid);
	[[nodiscard]] static CuboidSetType create(const PointType& point);
	friend struct CuboidSetConstIterator;
	friend struct CuboidSetConstView;
};
// CuboidSet is not marked final because AreaHasVisionCuboids derives from it.
struct CuboidSet : public CuboidSetBase<Cuboid, Point3D, CuboidSet>
{
	[[nodiscard]] static CuboidSet create(const Cuboid& cuboid);
	[[nodiscard]] static CuboidSet create(const Point3D& point);
	[[nodiscard]] static CuboidSet create([[maybe_unused]]const OffsetCuboid& spaceBoundry, const Point3D& pivot, const Facing4& newFacing, const OffsetCuboidSet& cuboids);
	[[nodiscard]] static CuboidSet create(const SmallSet<Point3D>& points);
};
struct OffsetCuboidSet final : public CuboidSetBase<OffsetCuboid, Offset3D, OffsetCuboidSet>
{
	[[nodiscard]] static OffsetCuboidSet create(const OffsetCuboid& cuboid);
	[[nodiscard]] static OffsetCuboidSet create(const Offset3D& point);
};

void to_json(Json& data, const CuboidSet& cuboidSet);
void from_json(const Json& data, CuboidSet& cuboidSet);

void to_json(Json& data, const OffsetCuboidSet& cuboidSet);
void from_json(const Json& data, OffsetCuboidSet& cuboidSet);

struct CuboidSetConstIterator final : public CuboidSetConstIteratorBase<Cuboid, Point3D, CuboidSet>{};
struct OffsetCuboidSetConstIterator final : public CuboidSetConstIteratorBase<OffsetCuboid, Offset3D, OffsetCuboidSet>{};