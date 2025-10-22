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
	CuboidSetType& operator=(CuboidSetType&& other) noexcept { m_cuboids = std::move(other.m_cuboids); return static_cast<CuboidSetType&>(*this); }
	CuboidSetType& operator=(const CuboidSetType& other) { m_cuboids = other.m_cuboids; return static_cast<CuboidSetType&>(*this); }
	void add(const PointType& point);
	void addAll(const CuboidSetType& other);
	void remove(const PointType& point);
	void removeAll(const auto& cuboids) { for(const CuboidType& cuboid : cuboids) remove(cuboid); }
	void removeContainedAndFragmentIntercepted(const CuboidType& cuboid);
	void removeContainedAndFragmentInterceptedAll(const CuboidSetType& cuboids);
	// TODO: why virtual?
	virtual void add(const CuboidType& cuboid);
	virtual void remove(const CuboidType& cuboid);
	void clear() { m_cuboids.clear(); }
	void shift(const Offset3D offset, const Distance& distance);
	// For merging with other cuboid sets.
	void addSet(const CuboidSetType& other);
	void rotateAroundPoint(const PointType& point, const Facing4& rotation);
	void reserve(const uint16_t& capacity) { m_cuboids.reserve(capacity); }
	void swap(CuboidSetType& other);
	void popBack();
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
	[[nodiscard]] PointType intersectionPoint(const CuboidType& cuboid) const;
	[[nodiscard]] bool intersects(const CuboidType& cuboid) const;
	[[nodiscard]] bool intersects(const CuboidSetType& cuboid) const;
	[[nodiscard]] bool isTouching(const CuboidType& cuboid) const;
	[[nodiscard]] bool isTouchingFace(const CuboidType& cuboid) const;
	[[nodiscard]] bool isTouchingFaceFromInside(const CuboidType& cuboid) const;
	[[nodiscard]] bool isTouching(const CuboidSetType& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(const CuboidSetType& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(const CuboidType& cuboid) const;
	[[nodiscard]] __attribute__((noinline)) std::string toString() const;
	[[nodiscard]] static CuboidSetType create(const SmallSet<PointType>& space);
	friend struct CuboidSetConstIterator;
	friend struct CuboidSetConstView;
};
// CuboidSet is not marked final because AreaHasVisionCuboids derives from it.
struct CuboidSet : public CuboidSetBase<Cuboid, Point3D, CuboidSet>
{
	[[nodiscard]] static CuboidSet create(const Cuboid& cuboid);
	[[nodiscard]] static CuboidSet create([[maybe_unused]]const OffsetCuboid& spaceBoundry, const Point3D& pivot, const Facing4& newFacing, const OffsetCuboidSet& cuboids);
};
struct OffsetCuboidSet final : public CuboidSetBase<OffsetCuboid, Offset3D, OffsetCuboidSet> { };

void to_json(Json& data, const CuboidSet& cuboidSet);
void from_json(const Json& data, CuboidSet& cuboidSet);

void to_json(Json& data, const OffsetCuboidSet& cuboidSet);
void from_json(const Json& data, OffsetCuboidSet& cuboidSet);

struct CuboidSetConstIterator final : public CuboidSetConstIteratorBase<Cuboid, Point3D, CuboidSet>{};
struct OffsetCuboidSetConstIterator final : public CuboidSetConstIteratorBase<OffsetCuboid, Offset3D, OffsetCuboidSet>{};