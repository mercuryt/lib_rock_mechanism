#pragma once
#include "reference.h"
#include "types.h"
#include "config.h"
struct ItemType;
struct MaterialType;
struct MaterialTypeCategory;
class Area;
class ItemQuery final
{
public:
	// Store Reference rather then Index because project saves query.
	ItemReference m_item;
	const ItemType* m_itemType = nullptr;
	const MaterialTypeCategory* m_materialTypeCategory = nullptr;
	const MaterialType* m_materialType = nullptr;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(const ItemReference& item);
	ItemQuery(const ItemType& m_itemType);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory& mtc);
	ItemQuery(const ItemType& m_itemType, const MaterialType& mt);
	ItemQuery(const ItemType& m_itemType, const MaterialType* mt);
	ItemQuery(const ItemType& m_itemType, const MaterialTypeCategory* mtc, const MaterialType* mt);
	ItemQuery(const Json& data, Area& area);
	void specalize(Area& area, ItemIndex item);
	void specalize(const MaterialType& materialType);
	[[nodiscard]] bool query(Area& area, const ItemIndex item) const;
	[[nodiscard]] bool operator==(const ItemQuery& itemQuery) const;
};
void to_json(Json& data, const ItemQuery&);
