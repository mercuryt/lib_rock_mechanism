#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
void ContextMenu::drawDigControls(BlockIndex& block)
{
	if(m_window.getFaction() && block.hasDesignation(*m_window.getFaction(), BlockDesignation::Dig))
	{
		auto cancelButton = tgui::Button::create("cancel dig");
		m_root.add(cancelButton);
		cancelButton->onClick([this]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
				if(selectedBlock->hasDesignation(*m_window.getFaction(), BlockDesignation::Dig))
					m_window.getArea()->m_hasDigDesignations.undesignate(*m_window.getFaction(), *selectedBlock);
			hide();
		});
	}
	else
	{
		// Dig.
		auto digButton = tgui::Button::create("dig");
		digButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(digButton);
		digButton->onClick([this]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
			{
				if(m_window.m_editMode)
				{
					if(selectedBlock->isSolid())
						selectedBlock->setNotSolid();
					else if(!selectedBlock->m_hasBlockFeatures.empty())
						selectedBlock->m_hasBlockFeatures.removeAll();
				}
				else if(selectedBlock->isSolid() || !selectedBlock->m_hasBlockFeatures.empty())
					m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), *selectedBlock, nullptr);
			}
			hide();
		});
		digButton->onMouseEnter([this]{
			auto& subMenu = makeSubmenu(0);
			auto blockFeatureTypeUI = widgetUtil::makeBlockFeatureTypeSelectUI();
			subMenu.add(blockFeatureTypeUI);
			auto button = tgui::Button::create("dig out feature");
			subMenu.add(button);
			button->onClick([this]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				assert(widgetUtil::lastSelectedBlockFeatureType);
				const BlockFeatureType& featureType = *widgetUtil::lastSelectedBlockFeatureType;
				for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
					if(selectedBlock->isSolid())
					{
						if(m_window.m_editMode)
							selectedBlock->m_hasBlockFeatures.hew(featureType);
						else
							m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), *selectedBlock, &featureType);
					}
				hide();
			});
		});
	}
}
