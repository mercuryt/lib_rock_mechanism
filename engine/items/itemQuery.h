#pragma once
#include "reference.h"
#include "types.h"
#include "json.h"
class Area;
class ItemQuery final
{
public:
	// Store Reference rather then Index because project saves query.
	ItemReference m_item;
	ItemTypeId m_itemType;
	MaterialCategoryTypeId m_materialTypeCategory;
	MaterialTypeId m_materialType;
	// To be used when inserting workpiece to project unconsumed items.
	ItemQuery(const ItemReference& item);
	ItemQuery(const ItemTypeId& m_itemType);
	ItemQuery(const ItemTypeId& m_itemType, const MaterialCategoryTypeId& mtc);
	ItemQuery(const ItemTypeId& m_itemType, const MaterialTypeId& mt);
	ItemQuery(const ItemTypeId& m_itemType, const MaterialCategoryTypeId& mtc, const MaterialTypeId& mt);
	ItemQuery(const Json& data, Area& area);
	void specalize(Area& area, const ItemIndex& item);
	void specalize(const MaterialTypeId& materialType);
	[[nodiscard]] bool query(Area& area, const ItemIndex& item) const;
	[[nodiscard]] bool operator==(const ItemQuery& itemQuery) const;
	[[nodiscard]] Json toJson() const;
};
