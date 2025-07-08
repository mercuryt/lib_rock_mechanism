#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
#include "../../engine/fluidType.h"
void ContextMenu::drawFluidControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	// Fluids.
	for(FluidData& fluidData : space.fluid_getAll(point))
	{
		auto label = tgui::Label::create(FluidType::getName(fluidData.type));
		label->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, &space, point, fluidData]{
			auto& submenu = makeSubmenu(0);
			auto remove = tgui::Button::create("remove");
			submenu.add(remove);
			remove->onClick([this, fluidData, &space]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const Point3D& selectedBlock : m_window.getSelectedBlocks().getView(space))
				{
					CollisionVolume contains = space.fluid_volumeOfTypeContains(selectedBlock, fluidData.type);
					if(contains != 0)
						space.fluid_removeSyncronus(selectedBlock, contains, fluidData.type);
				}
				hide();
			});
			FluidGroup& group = *space.fluid_getGroup(point, fluidData.type);
			if(group.getPoints().size() > 1)
			{
				auto selectGroup = tgui::Button::create("select adjacent");
				submenu.add(selectGroup);
				selectGroup->onClick([this, fluidData, &space, point]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(space.fluid_volumeOfTypeContains(point, fluidData.type) != 0)
					{
						FluidGroup& group = *space.fluid_getGroup(point, fluidData.type);
						m_window.deselectAll();
						for(const Point3D& point : group.getPoints())
							m_window.getSelectedBlocks().add(space, point);
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
		static CollisionVolume fluidLevel = Config::maxPointVolume;
		createFluidSource->onMouseEnter([this, &space, point]{
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
			levelUI->setMaximum(Config::maxPointVolume.get());
			levelUI->onValueChange([](const float value){fluidLevel = CollisionVolume::create(value);});
			levelUI->setValue(fluidLevel.get());
			submenu.add(levelUI);
			auto createFluid = tgui::Button::create("create fluid");
			createFluid->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(createFluid);
			createFluid->onClick([this, &space, point, fluidTypeUI, levelUI]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(point);
				for(const Point3D& selectedBlock : m_window.getSelectedBlocks().getView(space))
					if(!space.solid_is(selectedBlock))
						space.fluid_add(selectedBlock, fluidLevel, FluidType::byName(fluidTypeUI->getSelectedItemId().toWideString()));
				m_window.getArea()->m_hasFluidGroups.clearMergedFluidGroups();
				hide();
			});
			if(!m_window.getArea()->m_fluidSources.contains(point))
			{
				auto createSource = tgui::Button::create("create source");
				createSource->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				submenu.add(createSource);
				createSource->onClick([this, point, fluidTypeUI, levelUI, &space]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(point);
					for(const Point3D& selectedBlock: m_window.getSelectedBlocks().getView(space))
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
	if(m_window.getArea()->m_fluidSources.contains(point))
	{
		auto removeFluidSourceButton = tgui::Label::create("remove fluid source");
		removeFluidSourceButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(removeFluidSourceButton);
		removeFluidSourceButton->onClick([this, point]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			m_window.getArea()->m_fluidSources.destroy(point);
			hide();
		});
	}

}
