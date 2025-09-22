/*
 *  Polymorphic type for either an Actor or Item.
 *  TODO: bitbash
 */
#pragma once
#include "numericTypes/types.h"
#include "config.h"
#include "dishonorCallback.h"
#include "numericTypes/index.h"
#include "geometry/mapWithCuboidKeys.h"
#include <compare>
#include <functional>
#include <iterator>
class CanReserve;
class CanLead;
class CanFollow;
class Area;
struct Shape;
class ActorOrItemReference;
struct Point3D;
class ActorOrItemIndex
{
	HasShapeIndex m_index;
	bool m_isActor = false;
	ActorOrItemIndex(const HasShapeIndex& i, bool isA) : m_index(i), m_isActor(isA) { }
	static void setActorBit(HasShapeIndex& index);
	static void unsetActorBit(HasShapeIndex& index);
	[[nodiscard]] static bool getActorBit(const HasShapeIndex& index);
public:
	ActorOrItemIndex() = default;
	ActorOrItemIndex(const ActorOrItemIndex& other) : m_index(other.m_index), m_isActor(other.m_isActor) { }
	ActorOrItemIndex& operator=(const ActorOrItemIndex& other) { m_index = other.m_index; m_isActor = other.m_isActor; return *this; }
	static ActorOrItemIndex createForActor(const ActorIndex& actor) { return ActorOrItemIndex(actor, true); }
	static ActorOrItemIndex createForItem(const ItemIndex& item) { return ActorOrItemIndex(item, false); }
	static ActorOrItemIndex create(const ItemIndex& item) { return createForItem(item); }
	static ActorOrItemIndex create(const ActorIndex& actor) { return createForActor(actor); }
	static ActorOrItemIndex null() { return ActorOrItemIndex(); }
	void clear() { m_index.clear(); m_isActor = false; }
	void updateIndex(const HasShapeIndex& index) { m_index = index; }
	ActorOrItemIndex location_set(Area& area, const Point3D& location, const Facing4& facing) const;
	void location_clear(Area& area) const;
	void location_clearStatic(Area& area) const;
	void location_clearDynamic(Area& area) const;
	void followActor(Area& area, const ActorIndex& actor) const;
	void followItem(Area& area, const ItemIndex& item) const;
	void followPolymorphic(Area& area, const ActorOrItemIndex& actorOrItem) const;
	void unfollow(Area& area) const;
	void move_updateIndividualSpeed(Area& area) const;
	void maybeSetStatic(Area& area) const;
	void setStatic(Area& area) const;
	std::string toString() const;
	[[nodiscard]] ActorIndex getActor() const { assert(isActor()); return ActorIndex::create(m_index.get()); }
	[[nodiscard]] ItemIndex getItem() const { assert(isItem()); return ItemIndex::create(m_index.get()); }
	[[nodiscard]] ActorOrItemReference toReference(Area& area) const;
	[[nodiscard]] bool exists() const { return m_index.exists(); }
	[[nodiscard]] bool empty() const { return m_index.empty(); }
	[[nodiscard]] HasShapeIndex get() const { return m_index; }
	[[nodiscard]] bool isActor() const { return m_isActor; }
	[[nodiscard]] bool isItem() const { return !m_isActor; }
	[[nodiscard]] bool isStatic(Area& area) const;
	[[nodiscard]] bool isFollowing(Area& area) const;
	[[nodiscard]] bool isLeading(Area& area) const;
	[[nodiscard]] ActorOrItemIndex getFollower(Area& area) const;
	[[nodiscard]] ActorOrItemIndex getLeader(Area& area) const;

	[[nodiscard]] bool canEnterCurrentlyFrom(Area& area, const Point3D& destination, const Point3D& origin) const;
	[[nodiscard]] bool canEnterCurrentlyFromWithOccupied(Area& area, const Point3D& destination, const Point3D& origin, const CuboidSet& occupied) const;

	[[nodiscard]] Point3D getLocation(const Area& area) const;
	[[nodiscard]] const CuboidSet& getOccupied(const Area& area) const;
	[[nodiscard]] const MapWithCuboidKeys<CollisionVolume>& getOccupiedWithVolume(const Area& area) const;
	[[nodiscard]] CuboidSet getAdjacentCuboids(const Area& area) const;
	[[nodiscard]] bool isAdjacent(const Area& area, const ActorOrItemIndex& other) const;
	[[nodiscard]] bool isAdjacentToActor(const Area& area, const ActorIndex& other) const;
	[[nodiscard]] bool isAdjacentToItem(const Area& area, const ItemIndex& item) const;
	[[nodiscard]] bool isAdjacentToLocation(const Area& area, const Point3D& location) const;
	[[nodiscard]] bool occupiesPoint(const Area& area, const Point3D& location) const;

	[[nodiscard]] ShapeId getShape(const Area& area) const;
	[[nodiscard]] ShapeId getCompoundShape(const Area& area) const;
	[[nodiscard]] MoveTypeId getMoveType(const Area& area) const;
	[[nodiscard]] Mass getMass(const Area& area) const;
	[[nodiscard]] Quantity getQuantity(const Area& area) const;
	[[nodiscard]] Mass getSingleUnitMass(const Area& area) const;
	[[nodiscard]] FullDisplacement getVolume(const Area& area) const;
	[[nodiscard]] Facing4 getFacing(const Area& area) const;
	[[nodiscard]] bool isGeneric(const Area& area) const;
	[[nodiscard]] Point3D findAdjacentPointWithCondition(const Area& area, auto&& condition)
	{
		for(const Cuboid& cuboid : getAdjacentCuboids(area))
			for(const Point3D& point : cuboid)
				if(condition(point))
					return point;
		return Point3D::null();
	}
	[[nodiscard]] std::strong_ordering operator<=>(const ActorOrItemIndex& other) const;
	[[nodiscard]] bool operator==(const ActorOrItemIndex& other) const = default;
	struct Hash
	{
		[[nodiscard]] size_t operator()(const ActorOrItemIndex& actorOrItem) const;
	};
	void reservable_reserve(Area& area, CanReserve& canReserve, const Quantity quantity = Quantity::create(1), std::unique_ptr<DishonorCallback> callback = nullptr) const;
	void reservable_unreserve(Area& area, CanReserve& canReserve, const Quantity quantity = Quantity::create(1)) const;
	void reservable_maybeUnreserve(Area& area, CanReserve& canReserve, const Quantity quantity = Quantity::create(1)) const;
	void reservable_unreserveFaction(Area& area, const FactionId& faction) const;
	void validate(Area& area) const;
	[[nodiscard]] Quantity reservable_getUnreservedCount(const Area& area, const FactionId& faction) const;
	[[nodiscard]] bool reservable_exists(const Area& area, const FactionId& faction) const;
	[[nodiscard]] bool reservable_hasAny(const Area& area) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActorOrItemIndex, m_index, m_isActor);
};
