#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/blockFeature.h"
#include "../../engine/designations.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/areaBuilderUtil.h"
void ContextMenu::drawConstructControls(const BlockIndex& block)
{
	const Area& area = *m_window.getArea();
	const Blocks& blocks = area.getBlocks();
	const FactionId& faction = m_window.getFaction();
	if(faction.exists() && area.m_blockDesignations.contains(faction))
	{
		const auto& designations = area.m_blockDesignations.getForFaction(faction);
		if(designations.check(block, BlockDesignation::Construct))
		{
			auto cancelButton = tgui::Button::create("cancel");
			cancelButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			m_root.add(cancelButton);
			cancelButton->onClick([this, faction, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
					m_window.getArea()->m_hasConstructionDesignations.undesignate(faction, selectedBlock);
				hide();
			});
		}
	}
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
		auto materialTypeSelector = widgetUtil::makeMaterialSelectUI(widgetUtil::lastSelectedConstructionMaterial, L"", predicate);
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
			construct(block, constructed, widgetUtil::lastSelectedConstructionMaterial, widgetUtil::lastSelectedBlockFeatureType);
		});
	});
	constructButton->onClick([this, block]{
		construct(block, constructed, widgetUtil::lastSelectedConstructionMaterial, nullptr);
	});
}
void ContextMenu::construct(const BlockIndex& block, bool constructed, const MaterialTypeId& materialType, const BlockFeatureType* blockFeatureType)
{
	std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
	CuboidSet& selectedBlocks = m_window.getSelectedBlocks();
	if(selectedBlocks.empty())
		m_window.selectBlock(block);
	Blocks& blocks = m_window.getArea()->getBlocks();
	for(const Cuboid& cuboid : m_window.getSelectedBlocks().getCuboids())
	{
		if(m_window.m_editMode)
		{
			if(blockFeatureType == nullptr)
				blocks.solid_setCuboid(cuboid, materialType, constructed);
			else
				// TODO: Create block features by cuboid batch.
				for(const BlockIndex& selectedBlock : cuboid.getView(blocks))
				{
					if(!constructed)
					{
						if(!blocks.solid_is(selectedBlock))
							blocks.solid_set(selectedBlock, materialType, false);
						blocks.blockFeature_hew(selectedBlock, *blockFeatureType);
					}
					else
						blocks.blockFeature_construct(selectedBlock, *blockFeatureType, materialType);
				}
		}
		else
			for(const BlockIndex& selectedBlock : cuboid.getView(blocks))
				m_window.getArea()->m_hasConstructionDesignations.designate(m_window.getFaction(), selectedBlock, blockFeatureType, materialType);
	}
	hide();
}
