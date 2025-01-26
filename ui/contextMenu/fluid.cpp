#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/fluidType.h"
void ContextMenu::drawFluidControls(const BlockIndex& block)
{
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	// Fluids.
	for(FluidData& fluidData : blocks.fluid_getAll(block))
	{
		auto label = tgui::Label::create(FluidType::getName(fluidData.type));
		label->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, &blocks, block, fluidData]{
			auto& submenu = makeSubmenu(0);
			auto remove = tgui::Button::create("remove");
			submenu.add(remove);
			remove->onClick([this, fluidData, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
				{
					CollisionVolume contains = blocks.fluid_volumeOfTypeContains(selectedBlock, fluidData.type);
					if(contains != 0)
						blocks.fluid_removeSyncronus(selectedBlock, contains, fluidData.type);
				}
				hide();
			});
			FluidGroup& group = *blocks.fluid_getGroup(block, fluidData.type);
			if(group.getBlocks().size() > 1)
			{
				auto selectGroup = tgui::Button::create("select adjacent");
				submenu.add(selectGroup);
				selectGroup->onClick([this, fluidData, &blocks, block]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(blocks.fluid_volumeOfTypeContains(block, fluidData.type) != 0)
					{
						FluidGroup& group = *blocks.fluid_getGroup(block, fluidData.type);
						m_window.deselectAll();
						for(const BlockIndex& block : group.getBlocks())
							m_window.getSelectedBlocks().add(blocks, block);
					}
					hide();
				});
			}
		});
	}
	if(m_window.m_editMode)
	{
		auto createFluidSource = tgui::Label::create("create fluid");
		createFluidSource->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(createFluidSource);
		static CollisionVolume fluidLevel = Config::maxBlockVolume;
		createFluidSource->onMouseEnter([this, &blocks, block]{
			auto& submenu = makeSubmenu(0);
			auto fluidTypeLabel = tgui::Label::create("fluid type");
			submenu.add(fluidTypeLabel);
			fluidTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			auto fluidTypeUI = widgetUtil::makeFluidTypeSelectUI();
			submenu.add(fluidTypeUI);
			auto levelLabel = tgui::Label::create("level");
			levelLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(levelLabel);
			auto levelUI = tgui::SpinControl::create();
			levelUI->setMinimum(1);
			levelUI->setMaximum(Config::maxBlockVolume.get());
			levelUI->onValueChange([](const float value){fluidLevel = CollisionVolume::create(value);});
			levelUI->setValue(fluidLevel.get());
			submenu.add(levelUI);
			auto createFluid = tgui::Button::create("create fluid");
			createFluid->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(createFluid);
			createFluid->onClick([this, &blocks, block, fluidTypeUI, levelUI]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(block);
				for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
					if(!blocks.solid_is(selectedBlock))
						blocks.fluid_add(selectedBlock, fluidLevel, FluidType::byName(fluidTypeUI->getSelectedItemId().toWideString()));
				m_window.getArea()->m_hasFluidGroups.clearMergedFluidGroups();
				hide();
			});
			if(!m_window.getArea()->m_fluidSources.contains(block))
			{
				auto createSource = tgui::Button::create("create source");
				createSource->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				submenu.add(createSource);
				createSource->onClick([this, block, fluidTypeUI, levelUI, &blocks]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(const BlockIndex& selectedBlock: m_window.getSelectedBlocks().getView(blocks))
						m_window.getArea()->m_fluidSources.create(
							selectedBlock,
							FluidType::byName(fluidTypeUI->getSelectedItemId().toWideString()),
							fluidLevel
						);
					hide();
				});
			}
		});
	}
	if(m_window.getArea()->m_fluidSources.contains(block))
	{
		auto removeFluidSourceButton = tgui::Label::create("remove fluid source");
		removeFluidSourceButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(removeFluidSourceButton);
		removeFluidSourceButton->onClick([this, block]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			m_window.getArea()->m_fluidSources.destroy(block);
			hide();
		});
	}

}
