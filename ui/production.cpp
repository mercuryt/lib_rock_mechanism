#include "production.h"
#include "window.h"
#include "../engine/area.h"
#include "../engine/craft.h"
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
ProductionView::ProductionView(Window& window) : m_window(window), m_group(tgui::Group::create()), m_list(tgui::Grid::create()), m_newProductionView(window, *this), m_area(nullptr)
{
	m_window.getGui().add(m_group);
	m_group->setVisible(false);
	// Title.
	auto title = tgui::Label::create("Production Orders");
	m_group->add(title);
	// List of production orders.
	auto scrollable = tgui::ScrollablePanel::create();
	m_group->add(scrollable);
	scrollable->add(m_list);
	// Create.
	auto create = tgui::Button::create("+");
	m_group->add(create);
	create->onClick([&]{
		hide();
		m_newProductionView.show();
	});
	// Exit.
	auto exit = tgui::Button::create("x");
	m_group->add(exit);
	exit->onClick([&]{m_window.showGame();});
}
void ProductionView::display()
{
	size_t column = 0;
	for(CraftJob& craftJob : m_area->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction()).getAllJobs())
	{
		auto label = tgui::Label::create(m_window.displayNameForCraftJob(craftJob));
		m_list->addWidget(label, 1, column);
		auto cancel = tgui::Button::create("X");
		m_list->addWidget(cancel, 2, column);
		cancel->onClick([&]{m_area->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction()).removeJob(craftJob); });
		auto clone = tgui::Button::create("+");
		m_list->addWidget(clone, 3, column);
		clone->onClick([&]{m_area->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction()).cloneJob(craftJob); });
		++column;
	}
}

NewProductionView::NewProductionView(Window& window, ProductionView& productionView) : m_window(window), m_productionView(productionView), m_group(tgui::Group::create()), m_craftJobTypeSelector(tgui::ComboBox::create()), m_materialTypeSelector(tgui::ComboBox::create()), m_quantitySelector(tgui::SpinControl::create())
{

	m_window.getGui().add(m_group);
	m_group->setVisible(false);
	auto title = tgui::Label::create("New Production Order");
	m_group->add(title);
	// CraftJobType
	m_group->add(m_craftJobTypeSelector);
	for(CraftJobType& craftJobType : craftJobTypeDataStore)
		// Prodvide name as id paramater so we can localize the display name and use the real name as id.
		m_craftJobTypeSelector->addItem(craftJobType.name, craftJobType.name);
	m_craftJobTypeSelector->onItemSelect([&](const tgui::String& item){ m_craftJobType = &CraftJobType::byName(item.toStdString()); });
	// MaterialType
	m_group->add(m_materialTypeSelector);
	for(MaterialType& materialType : materialTypeDataStore)
		// Prodvide name as id paramater so we can localize the display name and use the real name as id.
		m_materialTypeSelector->addItem(materialType.name, materialType.name);
	m_materialTypeSelector->onItemSelect([&](const tgui::String& material){ m_materialType = &MaterialType::byName(material.toStdString()); });
	// Quantity
	m_group->add(m_quantitySelector);
	m_quantitySelector->setMinimum(0);
	m_quantitySelector->onValueChange([&](float value){ m_quantity = value; });
	// Confirm
	auto confirm = tgui::Button::create("+");
	m_group->add(confirm);
	confirm->onClick([&]{
		HasCraftingLocationsAndJobsForFaction& hasLocations = m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction());
		hasLocations.addJob(*m_craftJobType, m_materialType, m_quantity);
	});
	// Close
	auto close = tgui::Button::create("x");
	m_group->add(close);
	close->onClick([&]{
		hide();
		m_productionView.show();
	});
}
