#pragma once
#include "../config.h"
#include <unordered_set>
class Block;
struct Faction;
class DeserializationMemo;
class AreaHasSleepingSpots final
{
	std::unordered_set<Block*> m_unassigned;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void designate(Faction& faction, Block& block);
	void undesignate(Faction& faction, Block& block);
	bool containsUnassigned(const Block& block) const { return m_unassigned.contains(const_cast<Block*>(&block)); }
};
