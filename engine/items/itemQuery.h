#pragma once
#include "types.h"
#include "config.h"
struct ItemType;
struct MaterialType;
struct MaterialTypeCategory;
struct DeserializationMemo;
class Area;
class ItemQuery final
{
public:
	ItemIndex m_item = ITEM_INDEX_MAX;
	const ItemType* m_itemType = nullptr;
	const MaterialTypeCategory* m_materialTypeCategory = nullptr;
	const MaterialType* m_materialType = nullptr;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(ItemIndex item);
	ItemQuery(const ItemType& m_itemType);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc);
	ItemQuery(const ItemType& m_itemType, const MaterialType& mt);
	ItemQuery(const ItemType& m_itemType, const MaterialType* mt);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory* mtc, const MaterialType* mt);
	ItemQuery(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void specalize(Area& area, ItemIndex item);
	void specalize(const MaterialType& materialType);
	[[nodiscard]] bool query(Area& area, const ItemIndex item) const;
	[[nodiscard]] bool operator==(const ItemQuery& itemQuery) const;
};
