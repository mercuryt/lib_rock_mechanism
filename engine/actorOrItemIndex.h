/*
 *  Polymorphic type for either an Actor or Item.
 *  TODO: bitbash
 */
#pragma once
#include "types.h"
#include "config.h"
#include "dishonorCallback.h"
#include "index.h"
#include <compare>
#include <functional>
#include <iterator>
class CanReserve;
class CanLead;
class CanFollow;
class Area;
struct Shape;
struct MoveType;
struct Faction;
class ActorOrItemReference;
class ActorOrItemIndex
{
	HasShapeIndex m_index;
	bool m_isActor = false;
	ActorOrItemIndex(HasShapeIndex i, bool isA) : m_index(i), m_isActor(isA) { }
	static void setActorBit(HasShapeIndex& index);
	static void unsetActorBit(HasShapeIndex& index);
	[[nodiscard]] static bool getActorBit(HasShapeIndex& index);
public:
	ActorOrItemIndex() = default;
	static ActorOrItemIndex createForActor(ActorIndex actor) { return ActorOrItemIndex(actor, true); }
	static ActorOrItemIndex createForItem(ItemIndex item) { return ActorOrItemIndex(item, false); }
	void clear() { m_index.clear(); m_isActor = false; }
	void updateIndex(HasShapeIndex index) { m_index = index; }
	void setLocationAndFacing(Area& area, BlockIndex location, Facing facing) const;
	void followActor(Area& area, ActorIndex actor) const;
	void followItem(Area& area, ItemIndex item) const;
	void followPolymorphic(Area& area, ActorOrItemIndex actorOrItem) const;
	void unfollow(Area& area) const;
	[[nodiscard]] ActorOrItemReference toReference(Area& area);
	[[nodiscard]] bool exists() const { return m_index.exists(); }
	[[nodiscard]] HasShapeIndex get() const { return m_index; }
	[[nodiscard]] bool isActor() const { return m_isActor; }
	[[nodiscard]] bool isItem() const { return !m_isActor; }

	[[nodiscard]] bool isFollowing(Area& area) const;
	[[nodiscard]] bool isLeading(Area& area) const;
	[[nodiscard]] ActorOrItemIndex getFollower(Area& area) const;
	[[nodiscard]] ActorOrItemIndex getLeader(Area& area) const;
	[[nodiscard]] bool leaderCanMove(Area& area) const;

	[[nodiscard]] BlockIndex getLocation(const Area& area) const;
	[[nodiscard]] const BlockIndices& getBlocks(Area& area) const;
	[[nodiscard]] BlockIndices getAdjacentBlocks(Area& area) const;
	[[nodiscard]] bool isAdjacent(const Area& area, ActorOrItemIndex other) const;
	[[nodiscard]] bool isAdjacentToActor(const Area& area, ActorIndex other) const;
	[[nodiscard]] bool isAdjacentToItem(const Area& area, ItemIndex item) const;
	[[nodiscard]] bool isAdjacentToLocation(const Area& area, BlockIndex location) const;

	[[nodiscard]] const Shape& getShape(const Area& area) const;
	[[nodiscard]] const MoveType& getMoveType(const Area& area) const;
	[[nodiscard]] Mass getMass(const Area& area) const;
	[[nodiscard]] Mass getSingleUnitMass(const Area& area) const;
	[[nodiscard]] Volume getVolume(const Area& area) const;
	[[nodiscard]] Facing getFacing(const Area& area) const;
	[[nodiscard]] bool isGeneric(const Area& area) const;
	[[nodiscard]] std::strong_ordering operator<=>(const ActorOrItemIndex& other) const;
	[[nodiscard]] bool operator==(const ActorOrItemIndex& other) const = default;
	struct Hash
	{
		[[nodiscard]] size_t operator()(const ActorOrItemIndex& actorOrItem) const;
	};
	void reservable_reserve(Area& area, CanReserve& canReserve, Quantity quantity = 0, std::unique_ptr<DishonorCallback> callback = nullptr) const;
	void reservable_unreserve(Area& area, CanReserve& canReserve, Quantity quantity = 0) const;
	void reservable_maybeUnreserve(Area& area, CanReserve& canReserve, Quantity quantity = 0) const;
	void reservable_unreserveFaction(Area& area, const FactionId faction) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(Area& area, const FactionId faction) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActorOrItemIndex, m_index, m_isActor);
};
