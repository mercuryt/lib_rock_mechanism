#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
void ContextMenu::drawDigControls(const BlockIndex& block)
{
	const FactionId& faction = m_window.getFaction();
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	if(faction.exists())
	{
		auto& designations = area.m_blockDesignations.getForFaction(faction);
		if(designations.check(block, BlockDesignation::Dig))
		{
			auto cancelButton = tgui::Button::create("cancel dig");
			m_root.add(cancelButton);
			cancelButton->onClick([this, &designations]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks())
					if(designations.check(selectedBlock, BlockDesignation::Dig))
						m_window.getArea()->m_hasDigDesignations.undesignate(m_window.getFaction(), selectedBlock);
				hide();
			});
		}
	}
	else
	{
		// Dig.
		auto digButton = tgui::Button::create("dig");
		digButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(digButton);
		digButton->onClick([this, &blocks]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks())
			{
				if(m_window.m_editMode)
				{
					if(blocks.solid_is(selectedBlock))
						blocks.solid_setNot(selectedBlock);
					else if(!blocks.blockFeature_empty(selectedBlock))
						blocks.blockFeature_removeAll(selectedBlock);
				}
				else if(blocks.solid_is(selectedBlock) || !blocks.blockFeature_empty(selectedBlock))
					m_window.getArea()->m_hasDigDesignations.designate(m_window.getFaction(), selectedBlock, nullptr);
			}
			hide();
		});
		digButton->onMouseEnter([this, &blocks]{
			auto& subMenu = makeSubmenu(0);
			auto blockFeatureTypeUI = widgetUtil::makeBlockFeatureTypeSelectUI();
			subMenu.add(blockFeatureTypeUI);
			auto button = tgui::Button::create("dig out feature");
			subMenu.add(button);
			button->onClick([this, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				assert(widgetUtil::lastSelectedBlockFeatureType);
				const BlockFeatureType& featureType = *widgetUtil::lastSelectedBlockFeatureType;
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks())
					if(blocks.solid_is(selectedBlock))
					{
						if(m_window.m_editMode)
							blocks.blockFeature_hew(selectedBlock, featureType);
						else
							m_window.getArea()->m_hasDigDesignations.designate(m_window.getFaction(), selectedBlock, &featureType);
					}
				hide();
			});
		});
	}
}
