#include "production.h"
#include "widgets.h"
#include "window.h"
#include "../engine/area.h"
#include "../engine/craft.h"
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
NewProductionView::NewProductionView(Window& window, ProductionView& productionView) : m_window(window), m_productionView(productionView) { }
void NewProductionView::hide()
{
	m_window.getGui().remove(m_panel);
	m_panel = nullptr;
}
void NewProductionView::draw()
{
	m_panel = tgui::Panel::create();
	m_window.getGui().add(m_panel);
	// Title.
	auto title = tgui::Label::create("Create Production Order");
	m_panel->add(title);
	auto grid = tgui::Grid::create();
	m_panel->add(grid);
	grid->setPosition(tgui::bindLeft(title), tgui::bindBottom(title) + 10);
	// JobType and MaterialType.
	auto [craftJobTypeUI, materialTypeUI] = widgetUtil::makeCraftJobTypeAndMaterialTypeUI();
	grid->addWidget(tgui::Label::create("job type"), 0, 0);
	grid->addWidget(craftJobTypeUI, 0, 1);
	grid->addWidget(tgui::Label::create("material"), 1, 0);
	grid->addWidget(materialTypeUI, 1, 1);
	// Quantity.
	grid->addWidget(tgui::Label::create("quantity"), 2, 0);
	auto quantityUI = tgui::SpinControl::create();
	quantityUI->setValue(m_quantity);
	quantityUI->onValueChange([this](const auto value){ m_quantity = value; });
	grid->addWidget(quantityUI, 2, 1);
	// Confirm.
	auto confirm = tgui::Button::create("confirm");
	m_panel->add(confirm);
	confirm->onClick([&]{
		m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction()).
				addJob(*widgetUtil::lastSelectedCraftJobType, widgetUtil::lastSelectedMaterial, m_quantity);
		m_productionView.draw();
		hide();
	});
	confirm->setPosition(tgui::bindLeft(grid), tgui::bindBottom(grid) + 10);
	// Exit.
	auto exit = tgui::Button::create("close");
	m_panel->add(exit);
	exit->onClick([&]{
		m_productionView.draw();
		hide();
	});
	exit->setPosition(tgui::bindRight(confirm) + 10, tgui::bindBottom(grid) + 10);
}
ProductionView::ProductionView(Window& window) : m_window(window), m_newProductionView(window, *this) { }
void ProductionView::hide()
{
	m_window.getGui().remove(m_panel);
	m_panel = nullptr;
}
void ProductionView::draw()
{
	m_panel = tgui::Panel::create();
	m_window.getGui().add(m_panel);
	// Title.
	auto title = tgui::Label::create("Production Orders");
	m_panel->add(title);
	// Create.
	auto create = tgui::Button::create("create");
	m_panel->add(create);
	create->onClick([&]{
		m_newProductionView.draw();
		hide();	
	});
	create->setPosition(tgui::bindLeft(title), tgui::bindBottom(title) + 10);
	// Exit.
	auto exit = tgui::Button::create("close");
	m_panel->add(exit);
	exit->onClick([&]{m_window.showGame();});
	exit->setPosition(tgui::bindRight(create) + 10, tgui::bindTop(create));
	// List of production orders.
	auto list = tgui::Grid::create();
	auto scrollable = tgui::ScrollablePanel::create();
	m_panel->add(scrollable);
	scrollable->add(list);
	scrollable->setPosition(tgui::bindLeft(create), tgui::bindBottom(create) + 10);
	size_t row = 0;
	auto& hasCrafting = m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction());
	for(auto [craftJobType, materialType, quantity, craftJob] : collate())
	{
		auto label = tgui::Label::create(m_window.displayNameForCraftJob(*craftJob));
		list->addWidget(label, row, 1);
		if(quantity != 1)
			list->addWidget(tgui::Label::create(std::to_string(quantity)), row, 2);
		auto cancel = tgui::Button::create("-");
		list->addWidget(cancel, row, 3);
		cancel->onClick([this, &hasCrafting, craftJob]{
			hasCrafting.removeJob(*craftJob); 
			hide();
			draw();
		});
		auto clone = tgui::Button::create("+");
		list->addWidget(clone, row, 4);
		clone->onClick([this, &hasCrafting, craftJob]{
			hasCrafting.cloneJob(*craftJob); 
			hide();
			draw();
		});
		++row;
	}
}
std::vector<std::tuple<const CraftJobType*, const MaterialType*, Quantity, CraftJob*>> ProductionView::collate()
{
	std::vector<std::tuple<const CraftJobType*, const MaterialType*, Quantity, CraftJob*>> output;
	for(CraftJob& craftJob : m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction()).getAllJobs())
	{
		auto iter = std::ranges::find_if(output, [&](const auto& tuple){ 
			auto& [craftJobType, materialType, quantity, otherCraftJob] = tuple;
			return craftJobType == &craftJob.craftJobType && materialType == craftJob.materialType;
		});
		if(iter == output.end())
			output.emplace_back(&craftJob.craftJobType, craftJob.materialType, 1u, &craftJob);
		else
			++std::get<2>(*iter);
	}
	return output;
}
