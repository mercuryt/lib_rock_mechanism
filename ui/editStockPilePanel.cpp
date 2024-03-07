#include "editStockPilePanel.h"
#include "materialType.h"
#include "widgets.h"
#include "window.h"
#include "../engine/stockpile.h"

EditStockPileView::EditStockPileView(Window& window) : m_window(window), m_panel(tgui::Panel::create())
{
	hide();
	window.getGui().add(m_panel);
}

void EditStockPileView::draw(StockPile* stockpile)
{
	std::string verb = m_stockPile ? "edit" : "create";
	m_stockPile = stockpile;
	m_panel->removeAllWidgets();
	auto label = tgui::Label::create(verb + " stockpile");
	m_panel->add(label);
	uint index = 0;
	auto list = tgui::Grid::create();
	m_panel->add(list);
	list->setPosition(10, tgui::bindBottom(label) + 10);
	if(m_stockPile)
		for(ItemQuery& itemQuery : m_stockPile->getQueries())
		{
			list->addWidget(tgui::Label::create(itemQuery.m_itemType->name), index, 0);
			if(itemQuery.m_materialType)
				list->addWidget(tgui::Label::create(itemQuery.m_materialType->name), index, 1);
			if(itemQuery.m_materialTypeCategory)
				list->addWidget(tgui::Label::create(itemQuery.m_materialTypeCategory->name), index, 2);
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
	auto itemTypeUI = widgetUtil::makeItemTypeSelectUI();
	createGrid->addWidget(itemTypeUI, 0, 0);
	std::wstring emptyLabel = L"none";
	auto materialTypeUI = widgetUtil::makeMaterialSelectUI(emptyLabel);
	createGrid->addWidget(materialTypeUI, 1, 0);
	auto materialTypeCategoryUI = widgetUtil::makeMaterialCategorySelectUI(emptyLabel);
	createGrid->addWidget(materialTypeCategoryUI, 2, 0);
	//TODO: Why does this cause a hang?
	/*
	materialTypeUI->onItemSelect([materialTypeCategoryUI, emptyLabel]{
		if(widgetUtil::lastSelectedMaterial)
			materialTypeCategoryUI->setSelectedItemById(emptyLabel);
	});
	materialTypeCategoryUI->onItemSelect([materialTypeUI, emptyLabel](const tgui::String id){
		if(id != emptyLabel)
			materialTypeUI->setSelectedItemById(emptyLabel);
	});
	*/
	auto create = tgui::Button::create("create");
	m_panel->add(create);
	create->setPosition(10, tgui::bindBottom(createGrid) + 10);
	create->onClick([this, itemTypeUI, materialTypeUI, materialTypeCategoryUI]{
		if(!m_stockPile)
		{
			m_stockPile = &m_window.getArea()->m_hasStockPiles.at(*m_window.getFaction()).addStockPile({});
			for(Block* block : m_window.getSelectedBlocks())
				m_stockPile->addBlock(*block);
		}
		const MaterialType* materialType = widgetUtil::lastSelectedMaterial;
		const MaterialTypeCategory* materialTypeCategory = widgetUtil::lastSelectedMaterialCategory;
		const ItemType& itemType = *widgetUtil::lastSelectedItemType;
		ItemQuery query(itemType, materialTypeCategory, materialType);
		if(!m_stockPile->contains(query))
			m_window.getArea()->m_hasStockPiles.at(*m_window.getFaction()).addQuery(*m_stockPile, query);
		m_window.showEditStockPile(m_stockPile);
	});
	auto back = tgui::Button::create("back");
	m_panel->add(back);
	back->setPosition(10, tgui::bindBottom(create) + 10);
	back->onClick([this]{ m_window.showGame(); });
	show();
}
