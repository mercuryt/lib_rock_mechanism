#include "editStockPilePanel.h"
#include "materialType.h"
#include "widgets.h"
#include "window.h"
#include "../engine/stockpile.h"
#include "../engine/items/items.h"

EditStockPileView::EditStockPileView(Window& window) : m_window(window), m_panel(tgui::Panel::create())
{
	hide();
	window.getGui().add(m_panel);
}

void EditStockPileView::draw(StockPile* stockpile)
{
	std::wstring verb = m_stockPile ? L"edit" : L"create";
	m_stockPile = stockpile;
	m_panel->removeAllWidgets();
	auto label = tgui::Label::create(verb + L" stockpile");
	m_panel->add(label);
	uint index = 0;
	auto list = tgui::Grid::create();
	m_panel->add(list);
	list->setPosition(10, tgui::bindBottom(label) + 10);
	if(m_stockPile)
		for(ItemQuery& itemQuery : m_stockPile->getQueries())
		{
			list->addWidget(tgui::Label::create(ItemType::getName(itemQuery.m_itemType)), index, 0);
			if(itemQuery.m_materialType.exists())
				list->addWidget(tgui::Label::create(MaterialType::getName(itemQuery.m_materialType)), index, 1);
			if(itemQuery.m_materialTypeCategory.exists())
				list->addWidget(tgui::Label::create(MaterialTypeCategory::getName(itemQuery.m_materialTypeCategory)), index, 2);
			auto destroy = tgui::Button::create("destory");
			list->addWidget(destroy, index, 3);
			destroy->onClick([this, &itemQuery]{
				m_stockPile->removeQuery(itemQuery);
				m_window.showEditStockPile(m_stockPile);
			});
			++index;
		}
	auto createGrid = tgui::Grid::create();
	m_panel->add(createGrid);
	createGrid->setPosition(10, tgui::bindBottom(list) + 10);
	auto [itemTypeUI, materialTypeUI, materialTypeCategoryUI] = widgetUtil::makeItemTypeAndMaterialTypeOrMaterialTypeCategoryUI();
	createGrid->addWidget(itemTypeUI, 0, 0);
	createGrid->addWidget(materialTypeUI, 1, 0);
	createGrid->addWidget(materialTypeCategoryUI, 2, 0);
	auto create = tgui::Button::create("create");
	m_panel->add(create);
	create->setPosition(10, tgui::bindBottom(createGrid) + 10);
	create->onClick([this, itemTypeUI, materialTypeUI, materialTypeCategoryUI]{
		if(!m_stockPile)
		{
			const FactionId& faction = m_window.getFaction();
			m_window.getArea()->m_blockDesignations.maybeRegisterFaction(faction);
			m_stockPile = &m_window.getArea()->m_hasStockPiles.getForFaction(faction).addStockPile({});
			for(const BlockIndex& block : m_window.getSelectedBlocks())
				m_stockPile->addBlock(block);
		}
		const MaterialTypeId& materialType = widgetUtil::lastSelectedMaterial;
		const MaterialCategoryTypeId& materialTypeCategory = widgetUtil::lastSelectedMaterialCategory;
		const ItemTypeId& itemType = widgetUtil::lastSelectedItemType;
		auto query = ItemQuery::create(itemType, materialType, materialTypeCategory);
		if(!m_stockPile->contains(query))
			m_window.getArea()->m_hasStockPiles.getForFaction(m_window.getFaction()).addQuery(*m_stockPile, query);
		m_window.showEditStockPile(m_stockPile);
	});
	auto back = tgui::Button::create("back");
	m_panel->add(back);
	back->setPosition(10, tgui::bindBottom(create) + 10);
	back->onClick([this]{ m_window.showGame(); });
	show();
}
