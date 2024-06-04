#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
#include "blockFeature.h"
#include "designations.h"
#include "materialType.h"
void ContextMenu::drawConstructControls(BlockIndex& block)
{
	if(m_window.getFaction() && block.hasDesignation(*m_window.getFaction(), BlockDesignation::Construct))
	{
		auto cancelButton = tgui::Button::create("cancel");
		cancelButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(cancelButton);
		cancelButton->onClick([this]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
				m_window.getArea()->m_hasConstructionDesignations.undesignate(*m_window.getFaction(), *selectedBlock);
			hide();
		});
	}
	else 
	{
		auto constructButton = tgui::Button::create("construct");
		constructButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(constructButton);
		static bool constructed = false;
		constructButton->onMouseEnter([this, &block]{
			auto& subMenu = makeSubmenu(0);
			// Only list material types which have construction data, unless in edit mode.
			std::function<bool(const MaterialType&)> predicate = nullptr; 
			if(!m_window.m_editMode)
				predicate = [&](const MaterialType& materialType){ return materialType.constructionData; };
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
			constructFeature->onClick([this, &block]{
				construct(block, constructed, *widgetUtil::lastSelectedMaterial, widgetUtil::lastSelectedBlockFeatureType);
			});
		});
		constructButton->onClick([this, &block]{
			construct(block, constructed, *widgetUtil::lastSelectedMaterial, nullptr);
		});
	}
}
void ContextMenu::construct(BlockIndex& block, bool constructed, const MaterialType& materialType, const BlockFeatureType* blockFeatureType)
{
	std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
	if(m_window.getSelectedBlocks().empty())
		m_window.selectBlock(block);
	for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
		if(!selectedBlock->isSolid())
		{
			if (m_window.m_editMode)
			{
				if(blockFeatureType == nullptr)
					selectedBlock->setSolid(materialType, constructed);
				else
					if(!constructed)
					{
						if(!selectedBlock->isSolid())
							selectedBlock->setSolid(materialType);
						selectedBlock->m_hasBlockFeatures.hew(*blockFeatureType);
					}
					else
						selectedBlock->m_hasBlockFeatures.construct(*blockFeatureType, materialType);
			}
			else
				m_window.getArea()->m_hasConstructionDesignations.designate(*m_window.getFaction(), *selectedBlock, blockFeatureType, materialType);
		}
	hide();
}
