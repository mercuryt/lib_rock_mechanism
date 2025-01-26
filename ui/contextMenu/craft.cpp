#include "../contextMenu.h"
#include "../window.h"
#include "../../engine/blocks/blocks.h"
#include "../engine/types.h"
#include "craft.h"
#include "../displayData.h"
void ContextMenu::drawCraftControls(const BlockIndex& block)
{
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	if(m_window.getFaction().empty() || blocks.solid_is(block) || !blocks.shape_canStandIn(block))
		return;
	auto craftButton = tgui::Button::create("craft");
	m_root.add(craftButton);
	craftButton->onMouseEnter([this, block, &blocks]{
		auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction());
		auto categories = locationsAndJobsForFaction.getStepTypeCategoriesForLocation(block);
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
			button->onClick([this, category, block, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction());
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(block);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
					locationsAndJobsForFaction.removeLocation(category, selectedBlock);
				hide();
			});
		}
	});
	craftButton->onClick([this, block, &blocks]{
		std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
		auto& locationsAndJobsForFaction = m_window.getArea()->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction());
		if(m_window.getSelectedBlocks().empty())
			m_window.selectBlock(block);
		for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
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
