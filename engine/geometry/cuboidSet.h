#pragma once
#include "cuboid.h"
#include "offsetCuboid.h"
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
	void insertOrMerge(CuboidType cuboid);
	void destroy(const int cuboid);
	// For merging contained cuboids.
	void mergeInternal(CuboidType absorbed, const int absorber);
public:
	SmallSet<CuboidType> m_cuboids;
	CuboidSetBase() = default;
	CuboidSetBase(const CuboidSetType& other) : m_cuboids(other.m_cuboids) { }
	CuboidSetBase(CuboidSetType&& other) noexcept : m_cuboids(std::move(other.m_cuboids)) { }
	CuboidSetBase(PointType location, const Facing4 rotation, const OffsetCuboidSet& offsetPairs);
	CuboidSetBase(const std::initializer_list<CuboidType>& cuboids);
	CuboidSetBase(CuboidType cuboid) : m_cuboids({cuboid}) { }
	CuboidSetBase(const SmallSet<CuboidType>& cuboids) : m_cuboids(cuboids) { }
	CuboidSetBase(const SmallSet<PointType>& points) { for(PointType point : points) add(point); }
	CuboidSetType& operator=(CuboidSetType&& other) noexcept { m_cuboids = std::move(other.m_cuboids); return static_cast<CuboidSetType&>(*this); }
	CuboidSetType& operator=(const CuboidSetType& other) { m_cuboids = other.m_cuboids; return static_cast<CuboidSetType&>(*this); }
	void maybeAdd(PointType point);
	void maybeAddAll(const CuboidSetType& other);
	void maybeAdd(const CuboidSetType& other) { maybeAddAll(other); }
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
	void maybeRemove(PointType point);
	void maybeRemoveAll(const auto& cuboids) { for(CuboidType cuboid : cuboids) maybeRemove(cuboid); }
	void maybeRemove(const CuboidSetType& cuboids) { maybeRemoveAll(cuboids); }
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
	void maybeAdd(CuboidType cuboid);
	void maybeRemove(CuboidType cuboid);
	void clear() { m_cuboids.clear(); }
	void shift(Offset3D offset, Distance distance = {1});
	void shift(Facing6 facing, Distance distance = {1});
	CuboidSetType shifted(Offset3D offset, Distance distance = {1}) const;
	CuboidSetType shiftedDirection(Facing6 facing, Distance distance = {1}) const;
	void shiftWest(Distance distance = {1});
	void shiftSouth(Distance distance = {1});
	// For merging with other cuboid sets.
	void addSet(const CuboidSetType& other);
	void rotateAroundPoint(PointType point, Facing4 rotation);
	void reserve(int capacity) { m_cuboids.reserve(capacity); }
	void swap(CuboidSetType& other);
	void popBack();
	void inflate(Distance distance = {1});
	void inflateHorizontal(Distance distance = {1});
	void inflateVertical(Distance distance = {1});
	void inflateDirection(Facing6 facing, Distance distance = {1});
	void deflate(Distance distance = {1});
	void sliceAtZ(PointType::DimensionType zLevel);
	void prepare();
	[[nodiscard]] CuboidType operator[](int index) const { return m_cuboids[index]; }
	[[nodiscard]] CuboidType operator[](int index){ return m_cuboids[index]; }
	[[nodiscard]] PointType center() const;
	[[nodiscard]] PointType::DimensionType lowestZ() const;
	[[nodiscard]] PointType::DimensionType highestZ() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool exists() const;
	[[nodiscard]] int size() const;
	[[nodiscard]] int64_t volume() const;
	[[nodiscard]] bool contains(const Point3D point) const;
	[[nodiscard]] bool contains(const Offset3D point) const;
	[[nodiscard]] bool contains(CuboidType cuboid) const;
	[[nodiscard]] bool contains(const CuboidSetType& cuboid) const;
	[[nodiscard]] const auto& getCuboids() const { return m_cuboids; }
	[[nodiscard]] SmallSet<PointType> toPointSet() const;
	[[nodiscard]] bool isAdjacent(PointType cuboid) const;
	[[nodiscard]] bool isAdjacent(CuboidType cuboid) const;
	[[nodiscard]] CuboidType boundry() const;
	[[nodiscard]] PointType getLowest() const;
	[[nodiscard]] CuboidType front() const { return m_cuboids.front(); }
	[[nodiscard]] CuboidType back() const { return m_cuboids.back(); }
	[[nodiscard]] auto begin() { return m_cuboids.begin(); }
	[[nodiscard]] auto end() { return m_cuboids.end(); }
	[[nodiscard]] auto begin() const { return m_cuboids.begin(); }
	[[nodiscard]] auto end() const { return m_cuboids.end(); }
	[[nodiscard]] CuboidSetType intersection(CuboidType cuboid) const;
	[[nodiscard]] CuboidSetType intersection(const CuboidSetType& cuboid) const;
	[[nodiscard]] PointType intersectionPoint(CuboidType cuboid) const;
	[[nodiscard]] bool intersects(PointType point) const;
	[[nodiscard]] bool intersects(CuboidType cuboid) const;
	[[nodiscard]] bool intersects(const CuboidSetType& cuboid) const;
	[[nodiscard]] bool isTouching(CuboidType cuboid) const;
	[[nodiscard]] bool isTouchingFace(CuboidType cuboid) const;
	[[nodiscard]] bool isTouchingFaceFromInside(CuboidType cuboid) const;
	[[nodiscard]] bool isTouching(const CuboidSetType& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(const CuboidSetType& cuboids) const;
	[[nodiscard]] bool isIntersectingOrAdjacentTo(CuboidType cuboid) const;
	[[nodiscard]] CuboidType getCuboidContaining(PointType point) const;
	[[nodiscard]] CuboidSetType getAdjacent() const;
	[[nodiscard]] CuboidSetType getDirectlyAdjacent(Distance distance = {1}) const;
	[[nodiscard]] CuboidSetType inflateFaces(Distance distance = {1}) const;
	[[nodiscard]] CuboidSetType inflated(Distance distance = {1}) const;
	[[nodiscard]] CuboidSetType deflated(Distance distance = {1}) const;
	[[nodiscard]] CuboidSetType slicedAtZ(PointType::DimensionType zLevel) const;
	[[nodiscard]] CuboidSetType adjacentSlicedAtZ(PointType::DimensionType zLevel) const;
	[[nodiscard]] CuboidSetType flattened(PointType::DimensionType zLevel) const;
	[[nodiscard]] CuboidSetType getFace(const Facing6 facing) const;
	[[nodiscard]] CuboidSetType getFaceNorth() const;
	[[nodiscard]] CuboidSetType getFaceSouth() const;
	[[nodiscard]] CuboidSetType getFaceEast() const;
	[[nodiscard]] CuboidSetType getFaceWest() const;
	[[nodiscard]] CuboidSetType getFaceAbove() const;
	[[nodiscard]] CuboidSetType getFaceBelow() const;
	[[nodiscard]] CuboidSetType inverted() const;
	[[nodiscard]] PointType nearestPointTo(CuboidType cuboid) const;
	private:
	template<typename ShapeT>
	[[nodiscard]] CuboidSetType adjacentRecursiveBody(ShapeT shape) const;
	public:
	[[nodiscard]] CuboidSetType adjacentRecursive(CuboidType shape) const;
	[[nodiscard]] CuboidSetType adjacentRecursive(PointType shape) const;
	[[nodiscard]] bool isContiguous() const;
	[[nodiscard]] CuboidSetType getContainedWhichTouchesFromInside(CuboidType shape) const;
	[[nodiscard]] bool isEqual(const CuboidSetType& other) const;
	[[nodiscard]] int countIf(auto&& condition) const
	{
		int output = 0;
		for(const Cuboid cuboid : m_cuboids)
			output += cuboid.countIf(condition);
		return output;
	}
	[[nodiscard]] GDB_CALLABLE std::string toS() const;
	[[nodiscard]] static CuboidSetType create(const SmallSet<PointType>& space);
	[[nodiscard]] static CuboidSetType create(const CuboidSetType& set);
	[[nodiscard]] static CuboidSetType create(CuboidType cuboid);
	[[nodiscard]] static CuboidSetType create(PointType point);
	friend struct CuboidSetConstIterator;
	friend struct CuboidSetConstView;
};
struct CuboidSet final : public CuboidSetBase<Cuboid, Point3D, CuboidSet>
{
	[[nodiscard]] static CuboidSet create(const Cuboid cuboid);
	[[nodiscard]] static CuboidSet create(const Point3D point);
	[[nodiscard]] static CuboidSet create([[maybe_unused]]OffsetCuboid spaceBoundry, Point3D pivot, Facing4 newFacing, const OffsetCuboidSet& cuboids);
	[[nodiscard]] static CuboidSet create(const SmallSet<Point3D>& points);
	[[nodiscard]] static CuboidSet create(const std::vector<Point3D>& points);
	[[nodiscard]] static CuboidSet create(const SmallSet<Cuboid>& points);
};
struct OffsetCuboidSet final : public CuboidSetBase<OffsetCuboid, Offset3D, OffsetCuboidSet>
{
	[[nodiscard]] static OffsetCuboidSet create(OffsetCuboid cuboid);
	[[nodiscard]] static OffsetCuboidSet create(Offset3D point);
};

void to_json(Json& data, const CuboidSet& cuboidSet);
void from_json(const Json& data, CuboidSet& cuboidSet);

void to_json(Json& data, const OffsetCuboidSet& cuboidSet);
void from_json(const Json& data, OffsetCuboidSet& cuboidSet);

struct CuboidSetConstIterator final : public CuboidSetConstIteratorBase<Cuboid, Point3D, CuboidSet>{};
struct OffsetCuboidSetConstIterator final : public CuboidSetConstIteratorBase<OffsetCuboid, Offset3D, OffsetCuboidSet>{};