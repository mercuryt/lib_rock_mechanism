#pragma once
#include "types.h"
#include "config.h"
class Item;
struct ItemType;
struct MaterialType;
struct MaterialTypeCategory;
struct DeserializationMemo;
class ItemQuery final
{
public:
	Item* m_item;
	const ItemType* m_itemType;
	const MaterialTypeCategory* m_materialTypeCategory;
	const MaterialType* m_materialType;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(Item& item);
	ItemQuery(const ItemType& m_itemType);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc);
	ItemQuery(const ItemType& m_itemType, const MaterialType& mt);
	ItemQuery(const ItemType& m_itemType, const MaterialType* mt);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory* mtc, const MaterialType* mt);
	ItemQuery(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void specalize(Item& item);
	void specalize(const MaterialType& materialType);
	[[nodiscard]] bool query(const Item& item) const;
	[[nodiscard]] bool operator==(const ItemQuery& itemQuery) const;
};
