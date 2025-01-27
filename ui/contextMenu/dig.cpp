#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
void ContextMenu::drawDigControls(const BlockIndex& block)
{
	const FactionId& faction = m_window.getFaction();
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	if(faction.exists() && area.m_blockDesignations.contains(faction))
	{
		auto& designations = area.m_blockDesignations.getForFaction(faction);
		if(designations.check(block, BlockDesignation::Dig))
		{
			auto cancelButton = tgui::Button::create("cancel dig");
			m_root.add(cancelButton);
			cancelButton->onClick([this, &designations, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
					if(designations.check(selectedBlock, BlockDesignation::Dig))
						m_window.getArea()->m_hasDigDesignations.undesignate(m_window.getFaction(), selectedBlock);
				hide();
			});
		}
	}
	// Dig.
	auto digButton = tgui::Button::create("dig");
	digButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
	m_root.add(digButton);
	digButton->onClick([this, &blocks]{
		std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
		for(const Cuboid& cuboid : m_window.getSelectedBlocks().getCuboids())
		{
			if(m_window.m_editMode)
			{
				blocks.solid_setNotCuboid(cuboid);
				for(const BlockIndex& selectedBlock : cuboid.getView(blocks))
				{
					if(!blocks.blockFeature_empty(selectedBlock))
						blocks.blockFeature_removeAll(selectedBlock);
				}
			}
			else
			{
				for(const BlockIndex& selectedBlock : cuboid.getView(blocks))
					if(blocks.solid_is(selectedBlock) || !blocks.blockFeature_empty(selectedBlock))
						m_window.getArea()->m_hasDigDesignations.designate(m_window.getFaction(), selectedBlock, nullptr);
			}
		}
		hide();
	});
	digButton->onMouseEnter([this, &blocks, &area, block]{
		auto& subMenu = makeSubmenu(0);
		auto blockFeatureTypeUI = widgetUtil::makeBlockFeatureTypeSelectUI();
		subMenu.add(blockFeatureTypeUI);
		auto button = tgui::Button::create("dig out feature");
		subMenu.add(button);
		button->onClick([this, &blocks, &area, block]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			assert(widgetUtil::lastSelectedBlockFeatureType);
			const BlockFeatureType& featureType = *widgetUtil::lastSelectedBlockFeatureType;
			auto& selectedBlocks = m_window.getSelectedBlocks();
			if(selectedBlocks.empty())
				selectedBlocks.add(blocks, block);
			if(m_window.m_editMode)
			{
				for(const Cuboid& cuboid : selectedBlocks.getCuboids())
					for(const BlockIndex& block : cuboid.getView(blocks))
						if(blocks.solid_is(block))
							blocks.blockFeature_hew(block, featureType);
			}
			else
			{
				for(const Cuboid& cuboid : selectedBlocks.getCuboids())
					for(const BlockIndex& block : cuboid.getView(blocks))
						if(blocks.solid_is(block))
							m_window.getArea()->m_hasDigDesignations.designate(m_window.getFaction(), block, &featureType);
			}
			hide();
		});
	});
}
