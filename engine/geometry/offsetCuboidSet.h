#pragma once
#include "offsetCuboid.h"
#include "../dataStructures/smallSet.h"

struct OffsetCuboidSet
{
	SmallSet<OffsetCuboid> m_data;
	// Try to merge ane existing cuboid with each of the other cuboids.
	// Used for recursive merge.
	void maybeMergeExisting(SmallSet<OffsetCuboid>::iterator iter);
public:
	void addOrMerge(const OffsetCuboid& cuboid);
	void addOrMerge(const Offset3D& offset);
	void maybeRemove(const Offset3D& offset);
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] uint size() const
	{
		uint output = 0;
		for(const OffsetCuboid& cuboid : m_data)
			output += cuboid.size();
		return output;
	}
	class ConstIterator
	{
		OffsetCuboidSet* m_cuboidSet;
		SmallSet<OffsetCuboid>::const_iterator m_outerIter;
		OffsetCuboid::ConstIterator m_innerIter;
		bool m_end = false;
	public:
		ConstIterator() = default;
		ConstIterator(const OffsetCuboidSet& cuboidSet, bool end);
		ConstIterator& operator=(ConstIterator& other) = default;
		[[nodiscard]] bool operator==(const ConstIterator& other) const;
		[[nodiscard]] Offset3D operator*() const;
		ConstIterator& operator++();
		[[nodiscard]] ConstIterator operator++(int);
	};
	ConstIterator begin() const { return {*this, false}; }
	ConstIterator end() const { return {*this, true}; }
	auto cuboidsBegin() const { return m_data.begin(); }
	auto cuboidsEnd() const { return m_data.end(); }
	[[nodiscard]] Json toJson() const;
	void load(const Json& data);
};
inline void to_json(Json& data, const OffsetCuboidSet& set) { data = set.toJson(); }
inline void from_json(const Json& data, OffsetCuboidSet& set) { set.load(data); }