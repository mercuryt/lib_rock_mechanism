#pragma once
#include "reference.h"
#include "numericTypes/types.h"
#include "json.h"
class Area;
class ItemQuery final
{
public:
	// Store Reference rather then Index because project saves query.
	ItemReference m_item;
	ItemTypeId m_itemType;
	MaterialCategoryTypeId m_solidCategory;
	MaterialTypeId m_solid;
	ItemQuery() = default;
	ItemQuery(const Json& data, Area& area) { load(data, area); }
	// To be used when inserting workpiece to project unconsumed items.
	void specalize(Area& area, const ItemIndex& item);
	void specalize(const MaterialTypeId& materialType);
	void maybeSpecalize(const MaterialTypeId& materialType);
	void validate() const;
	void load(const Json& data, Area& area);
	[[nodiscard]] bool query(Area& area, const ItemIndex& item) const;
	[[nodiscard]] bool operator==(const ItemQuery& itemQuery) const;
	[[nodiscard]] Json toJson() const;
	static ItemQuery create(const ItemReference& item){ ItemQuery output; output.m_item = item; return output;}
	static ItemQuery create(const ItemTypeId& itemType){ ItemQuery output; output.m_itemType = itemType; return output;}
	static ItemQuery create(const ItemTypeId& itemType, const MaterialCategoryTypeId& mtc){ ItemQuery output; output.m_itemType = itemType; output.m_solidCategory = mtc; return output;}
	static ItemQuery create(const ItemTypeId& itemType, const MaterialTypeId& mt){ ItemQuery output; output.m_itemType = itemType; output.m_solid = mt; return output;}
	static ItemQuery create(const ItemTypeId& itemType, const MaterialTypeId& mt, const MaterialCategoryTypeId& mtc){ ItemQuery output; output.m_itemType = itemType; output.m_solid = mt; output.m_solidCategory = mtc; assert(mt == MaterialTypeId::null() || mtc == MaterialCategoryTypeId::null());return output;}
};
inline void to_json(Json& data, const ItemQuery& query) { data = query.toJson(); }