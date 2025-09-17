#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"

class Area;

class Support
{
	RTreeBoolean m_support;
	CuboidSet m_maybeFall;
public:
	void doStep(Area& area);
	void set(const auto& shape) { m_support.maybeInsert(shape); }
	void unset(const auto& shape) { m_support.maybeRemove(shape); }
	void maybeFall(const Cuboid& cuboid) { m_maybeFall.add(cuboid); }
	void prepare();
	[[nodiscard]] CuboidSet getUnsupported(const Area& area, const Cuboid& cuboid) const;
	[[nodiscard]] uint maybeFallVolume() const { return m_maybeFall.volume(); }
};