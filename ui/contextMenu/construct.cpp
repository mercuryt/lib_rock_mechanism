#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/blockFeature.h"
#include "../../engine/designations.h"
#include "../../engine/materialType.h"
void ContextMenu::drawConstructControls(const BlockIndex& block)
{
	const Area& area = *m_window.getArea();
	const FactionId& faction = m_window.getFaction();
	if(faction.exists())
	{
		const auto& designations = area.m_blockDesignations.getForFaction(faction);
		if(designations.check(block, BlockDesignation::Construct))
		{
			auto cancelButton = tgui::Button::create("cancel");
			cancelButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			m_root.add(cancelButton);
			cancelButton->onClick([this, faction]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks())
					m_window.getArea()->m_hasConstructionDesignations.undesignate(faction, selectedBlock);
				hide();
			});
		}
	}
	else
	{
		auto constructButton = tgui::Button::create("construct");
		constructButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(constructButton);
		static bool constructed = false;
		constructButton->onMouseEnter([this, block]{
			auto& subMenu = makeSubmenu(0);
			// Only list material types which have construction data, unless in edit mode.
			std::function<bool(const MaterialTypeId&)> predicate = nullptr;
			if(!m_window.m_editMode)
				predicate = [&](const MaterialTypeId& materialType)->bool{ return MaterialType::construction_getConsumed(materialType).empty(); };
			auto materialTypeSelector = widgetUtil::makeMaterialSelectUI(L"", predicate);
			subMenu.add(materialTypeSelector);
			if(m_window.m_editMode)
			{
				auto label = tgui::Label::create("constructed");
				subMenu.add(label);
				auto constructedUI = tgui::CheckBox::create();
				subMenu.add(constructedUI);
				constructedUI->setSize(10, 10);
				constructedUI->setChecked(constructed);
				constructedUI->onChange([](bool value){ constructed = value; });
			}
			auto blockFeatureTypeUI = widgetUtil::makeBlockFeatureTypeSelectUI();
			subMenu.add(blockFeatureTypeUI);
			auto constructFeature = tgui::Button::create("construct feature");
			subMenu.add(constructFeature);
			constructFeature->onClick([this, block]{
				construct(block, constructed, widgetUtil::lastSelectedMaterial, widgetUtil::lastSelectedBlockFeatureType);
			});
		});
		constructButton->onClick([this, block]{
			construct(block, constructed, widgetUtil::lastSelectedMaterial, nullptr);
		});
	}
}
void ContextMenu::construct(const BlockIndex& block, bool constructed, const MaterialTypeId& materialType, const BlockFeatureType* blockFeatureType)
{
	std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
	if(m_window.getSelectedBlocks().empty())
		m_window.selectBlock(block);
	Blocks& blocks = m_window.getArea()->getBlocks();
	for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks())
		if(!blocks.solid_is(selectedBlock))
		{
			if (m_window.m_editMode)
			{
				if(blockFeatureType == nullptr)
					blocks.solid_set(selectedBlock, materialType, constructed);
				else
					if(!constructed)
					{
						if(!blocks.solid_is(selectedBlock))
							blocks.solid_set(selectedBlock, materialType, false);
						blocks.blockFeature_hew(selectedBlock, *blockFeatureType);
					}
					else
						blocks.blockFeature_construct(selectedBlock, *blockFeatureType, materialType);
			}
			else
				m_window.getArea()->m_hasConstructionDesignations.designate(m_window.getFaction(), selectedBlock, blockFeatureType, materialType);
		}
	hide();
}
