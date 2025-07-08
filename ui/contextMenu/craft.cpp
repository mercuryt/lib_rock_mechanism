#include "../contextMenu.h"
#include "../window.h"
#include "../../engine/space/space.h"
#include "../engine/numericTypes/types.h"
#include "craft.h"
#include "../displayData.h"
void ContextMenu::drawCraftControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	if(m_window.getFaction().empty() || space.solid_is(point) || !space.shape_canStandIn(point))
		return;
	auto craftButton = tgui::Button::create("craft");
	m_root.add(craftButton);
	craftButton->onMouseEnter([this, point, &space]{
		auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction());
		auto categories = locationsAndJobsForFaction.getStepTypeCategoriesForLocation(point);
		auto& subMenu = makeSubmenu(0);
		auto speciesLabel = tgui::Label::create("craft job");
		speciesLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		subMenu.add(speciesLabel);
		auto stepTypeCategoryUI = widgetUtil::makeCraftStepTypeCategorySelectUI();
		subMenu.add(stepTypeCategoryUI);
		for(const CraftStepTypeCategoryId& category : categories)
		{
			auto button = tgui::Button::create(L"undesignate " + CraftStepTypeCategory::getName(category));
			subMenu.add(button);
			button->onClick([this, category, point, &space]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction());
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(point);
				for(const Point3D& selectedBlock : m_window.getSelectedBlocks().getView(space))
					locationsAndJobsForFaction.removeLocation(category, selectedBlock);
				hide();
			});
		}
	});
	craftButton->onClick([this, point, &space]{
		std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
		auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction());
		if(m_window.getSelectedBlocks().empty())
			m_window.selectBlock(point);
		for(const Point3D& selectedBlock : m_window.getSelectedBlocks().getView(space))
			locationsAndJobsForFaction.addLocation(widgetUtil::lastSelectedCraftStepTypeCategory, selectedBlock);
		hide();
	});
	auto productionButton = tgui::Button::create("production");
	m_root.add(productionButton);
	productionButton->onClick([this]{
		m_window.showProduction();
		hide();
	});
}
