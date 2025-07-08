#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "point3D.h"
class CuboidView;
class CuboidSurfaceView;
class Sphere;
class Cuboid
{
public:
	// TODO: remove the m_ prefix and make this a struct.
	Point3D m_highest;
	Point3D m_lowest;

	Cuboid(const Point3D& highest, const Point3D& lowest);

	Cuboid() = default;
	void merge(const Cuboid& cuboid);
	void setFrom(const Point3D& point);
	void setFrom(const Point3D& a, const Point3D& b);
	void clear();
	void shift(const Facing6& direction, const Distance& distance);
	void shift(const Offset3D& offset, const Distance& distance);
	void rotateAroundPoint(const Point3D& point, const Facing4& rotation);
	void setMaxZ(const Distance& distance);
	void maybeExpand(const Cuboid& other);
	void maybeExpand(const Point3D& point);
	[[nodiscard]] Cuboid inflateAdd(const Distance& distance) const;
	[[nodiscard]] SmallSet<Point3D> toSet() const;
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] bool contains(const Cuboid& cuboid) const;
	[[nodiscard]] bool canMerge(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid canMergeSteal(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid sum(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid intersection(const Cuboid& cuboid) const;
	[[nodiscard]] Cuboid getFace(const Facing6& faceing) const;
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] bool overlapsWithSphere(const Sphere& sphere) const;
	//TODO: rename size to volume.
	[[nodiscard]] uint size() const;
	[[nodiscard]] bool empty() const { return m_highest.empty(); }
	[[nodiscard]] bool exists() const { return m_highest.exists(); }
	[[nodiscard]] bool operator==(const Cuboid& cuboid) const;
	[[nodiscard]] Point3D getCenter() const;
	[[nodiscard]] Distance dimensionForFacing(const Facing6& facing) const;
	[[nodiscard]] Facing6 getFacingTwordsOtherCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] bool isSomeWhatInFrontOf(const Point3D& position, const Facing4& facing) const;
	// TODO: Should this return a CuboidArray<6>?
	[[nodiscard]] SmallSet<Cuboid> getChildrenWhenSplitByCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] std::pair<Cuboid, Cuboid> getChildrenWhenSplitBelowCuboid(const Cuboid& cuboid) const;
	[[nodiscard]] bool isTouching(const Cuboid& cuboid) const;
	[[nodiscard]] static Cuboid fromPoint(const Point3D& point);
	[[nodiscard]] static Cuboid fromPointPair(const Point3D& a, const Point3D& b);
	[[nodiscard]] static Cuboid fromPointSet(const SmallSet<Point3D>& set);
	[[nodiscard]] static Cuboid createCube(const Point3D& center, const Distance& width);
	class iterator
	{
	private:
		Point3D m_start;
		Point3D m_end;
		Point3D m_current;
		void setToEnd();
	public:
		iterator(const Point3D& low, const Point3D& high) : m_start(low), m_end(high), m_current(low) { }
		iterator(const iterator& other) = default;
		iterator& operator=(const iterator& other);
		iterator& operator++();
		[[nodiscard]] iterator operator++(int);
		[[nodiscard]] bool operator==(const iterator& other) const { return m_current == other.m_current; }
		[[nodiscard]] bool operator!=(const iterator& other) const { return !(*this == other); }
		[[nodiscard]] Point3D operator*();
	};
	iterator begin() { return iterator(m_lowest, m_highest); }
	iterator end() { return iterator(Point3D::null(), Point3D::null()); }
	const iterator begin() const { return iterator(m_lowest, m_highest); }
	const iterator end() const { return iterator(Point3D::null(), Point3D::null()); }
	//TODO:
	//static_assert(std::forward_iterator<iterator>);
	CuboidSurfaceView getSurfaceView() const;
	std::string toString() const;
	// Compares each pair of cuboids to find the combination where combined.size() - (pair1.size() + pair2.size()) is lowest.
	[[nodiscard]] static std::tuple<Cuboid, uint, uint> findPairWithLeastNewVolumeWhenExtended(auto& cuboids);
	struct Hash{
		size_t operator()(const Cuboid& cuboid) { return Point3D::Hash()(cuboid.m_highest); }
	};
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cuboid, m_highest, m_lowest);
};
struct CuboidSurfaceView : public std::ranges::view_interface<CuboidSurfaceView>
{
	const Cuboid cuboid;
	CuboidSurfaceView() = default;
	CuboidSurfaceView(const Cuboid c) : cuboid(c) { }
	struct Iterator
	{
		const CuboidSurfaceView& view;
		Cuboid face;
		Point3D current;
		Facing6 facing = Facing6::Below;
		void setFace();
		void setToEnd();
		Iterator(const CuboidSurfaceView& v);
		Iterator& operator++();
		Iterator operator++(int);
		bool operator==(const Iterator& other) const;
		bool operator!=(const Iterator& other) const { return !(*this == other); }
		std::pair<Point3D, Facing6> operator*();
	};
	Iterator begin() const { return {*this}; }
	Iterator end() const { Iterator output(*this); output.setToEnd(); return output; }
	//static_assert(std::forward_iterator<Iterator>);
};