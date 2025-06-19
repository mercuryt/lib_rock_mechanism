#pragma once

#include "../geometry/point3D.h"
#include "../geometry/cuboidSetSIMD.h"
#include "../dataStructures/strongVector.h"
#include "../strongInteger.h"
#include <array>

using Octant = uint8_t;

template<typename NodeData, uint splitThreashold, uint mergeThreashold>
class Bool3D
{
	class Index : public StrongInteger<Index, uint16_t>
	{
	public:
		Index() = default;
		struct Hash { [[nodiscard]] size_t operator()(const Index& index) const { return index.get(); } };
	};
	struct Node
	{
		NodeData data;
		Index childData;
	};
	struct EightNodes
	{
		std::array<Node, 8> nodes;
		CuboidArray<8> cuboids;
		Point3D center;
	};
	StrongVector<EightNodes, Index> m_nodes;
	StrongVector<Node*, Index> m_parents;
	Index m_nextIndex = Index::create(0);
	Point3D m_center;
	void split(Node& node, const Cuboid& cuboid);
	void merge(Node& node);
public:
	void maybeSetPoint(const Point3D& point);
	void maybeUnsetPoint(const Point3D& point);
	void maybeSetCuboid(const Cuboid& cuboid);
	void maybeUnsetCuboid(const Cuboid& cuboid);
	[[nodiscard]] bool queryPoint(const Point3D& point);
	[[nodiscard]] bool queryCuboidAny(const Cuboid& cuboid);
	[[nodiscard]] static Octant getOctant(const Point3D& center, const Point3D& point);
};