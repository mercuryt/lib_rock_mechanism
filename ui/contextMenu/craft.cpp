#include "../contextMenu.h"
#include "../window.h"
#include "../../engine/block.h"
#include "craft.h"
#include "designations.h"
#include "types.h"
void ContextMenu::drawCraftControls(Block& block)
{
	if(!m_window.getFaction() || block.isSolid() || !block.m_hasShapes.canStandIn())
		return;
	auto craftButton = tgui::Button::create("craft");
	m_root.add(craftButton);
	craftButton->onMouseEnter([this, &block]{
		auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction());
		auto categories = locationsAndJobsForFaction.getStepTypeCategoriesForLocation(block);
		auto& subMenu = makeSubmenu(0);
		auto stepTypeCategoryUI = widgetUtil::makeCraftStepTypeCategorySelectUI();
		subMenu.add(stepTypeCategoryUI);
		for(const CraftStepTypeCategory* category : categories)
		{
			auto button = tgui::Button::create("undesignate " + category->name);
			subMenu.add(button);
			button->onClick([this, category, &block]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction());
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(block);
				for(Block* selectedBlock : m_window.getSelectedBlocks())
					locationsAndJobsForFaction.removeLocation(*category, *selectedBlock);
				hide();
			});
		}
	});
	craftButton->onClick([this, &block]{
		std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
		auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction());
		if(m_window.getSelectedBlocks().empty())
			m_window.selectBlock(block);
		for(Block* selectedBlock : m_window.getSelectedBlocks())
			locationsAndJobsForFaction.addLocation(*widgetUtil::lastSelectedCraftStepTypeCategory, *selectedBlock);
		hide();
	});
	auto productionButton = tgui::Button::create("production");
	m_root.add(productionButton);
	productionButton->onClick([this]{
		m_window.showProduction();
		hide();
	});
}
