#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
void ContextMenu::drawFluidControls(Block& block)
{
	// Fluids.
	for(auto& [fluidType, pair] : block.m_hasFluids.getAll())
	{
		auto label = tgui::Label::create(fluidType->name);
		label->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, &block, fluidType]{
			auto& submenu = makeSubmenu(0);
			auto remove = tgui::Button::create("remove");
			submenu.add(remove);
			remove->onClick([this, fluidType]{ 
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(auto* selectedBlock : m_window.getSelectedBlocks())
				{
					Volume contains = selectedBlock->m_hasFluids.volumeOfFluidTypeContains(*fluidType);
					if(contains)
						selectedBlock->m_hasFluids.removeFluidSyncronus(contains, *fluidType);
				}
				hide();
			});
			FluidGroup& group = *block.m_hasFluids.getFluidGroup(*fluidType);
			if(group.getBlocks().size() > 1)
			{
				auto selectGroup = tgui::Button::create("select adjacent");
				submenu.add(selectGroup);
				selectGroup->onClick([this, fluidType, &block]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(block.m_hasFluids.volumeOfFluidTypeContains(*fluidType))
					{
						FluidGroup& group = *block.m_hasFluids.getFluidGroup(*fluidType);
						m_window.deselectAll();
						for(Block* block : group.getBlocks())
							m_window.getSelectedBlocks().insert(block);
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
		static Volume fluidLevel = Config::maxBlockVolume;
		createFluidSource->onMouseEnter([this, &block]{
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
			levelUI->setMaximum(Config::maxBlockVolumeHardLimit);
			levelUI->onValueChange([](const float value){fluidLevel = value;});
			levelUI->setValue(fluidLevel);
			submenu.add(levelUI);
			auto createFluid = tgui::Button::create("create fluid");
			createFluid->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(createFluid);
			createFluid->onClick([this, &block, fluidTypeUI, levelUI]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(block);
				for(Block* selectedBlock : m_window.getSelectedBlocks())
					if(!selectedBlock->isSolid())
						selectedBlock->m_hasFluids.addFluid(fluidLevel, FluidType::byName(fluidTypeUI->getSelectedItemId().toStdString()));
				m_window.getArea()->m_hasFluidGroups.clearMergedFluidGroups();
				hide();
			});
			if(!m_window.getArea()->m_fluidSources.contains(block))
			{
				auto createSource = tgui::Button::create("create source");
				createSource->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				submenu.add(createSource);
				createSource->onClick([this, &block, fluidTypeUI, levelUI]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block* selectedBlock: m_window.getSelectedBlocks())
						m_window.getArea()->m_fluidSources.create(
							*selectedBlock, 
							FluidType::byName(fluidTypeUI->getSelectedItemId().toStdString()), 
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
		removeFluidSourceButton->onClick([this, &block]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			m_window.getArea()->m_fluidSources.destroy(block);
			hide();
		});
	}

}
