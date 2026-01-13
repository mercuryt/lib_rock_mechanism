#pragma once
#include "../project.h"
#include "../reference.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"

#include <cstdio>
class InstallItemProject final : public Project
{
	const ItemReference m_item;
	Facing4 m_facing = Facing4::Null;
public:
	// Max one worker.
	InstallItemProject(Area& area, const ItemReference& i, const Point3D& l, const Facing4& facing, const FactionId& faction, const CuboidSet& occupied);
	void onComplete();
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const { return {{ItemQuery::create(m_item), Quantity::create(1)}}; }
	std::vector<ActorReference> getActors() const { return {}; }
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const { return {}; }
	[[nodiscard]] SkillTypeId getSkill() const { return SkillTypeId::null(); }
	[[nodiscard]] std::string description() const;
	Step getDuration() const { return Config::installItemDuration; }
	bool canReset() const { return false; }
	void onDelay() { cancel(); }
	void offDelay() { std::unreachable(); }
};