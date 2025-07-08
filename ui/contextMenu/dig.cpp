#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
void ContextMenu::drawDigControls(const Point3D& point)
{
	const FactionId& faction = m_window.getFaction();
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	if(faction.exists() && area.m_spaceDesignations.contains(faction))
	{
		auto& designations = area.m_spaceDesignations.getForFaction(faction);
		if(designations.check(point, SpaceDesignation::Dig))
		{
			auto cancelButton = tgui::Button::create("cancel dig");
			m_root.add(cancelButton);
			cancelButton->onClick([this, &designations, &space]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const Point3D& selectedBlock : m_window.getSelectedBlocks().getView(space))
					if(designations.check(selectedBlock, SpaceDesignation::Dig))
						m_window.getArea()->m_hasDigDesignations.undesignate(m_window.getFaction(), selectedBlock);
				hide();
			});
		}
	}
	// Dig.
	auto digButton = tgui::Button::create("dig");
	digButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
	m_root.add(digButton);
	digButton->onClick([this, &space]{
		std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
		for(const Cuboid& cuboid : m_window.getSelectedBlocks().getCuboids())
		{
			if(m_window.m_editMode)
			{
				space.solid_setNotCuboid(cuboid);
				for(const Point3D& selectedBlock : cuboid.getView(space))
				{
					if(!space.pointFeature_empty(selectedBlock))
						space.pointFeature_removeAll(selectedBlock);
				}
			}
			else
			{
				for(const Point3D& selectedBlock : cuboid.getView(space))
					if(space.solid_is(selectedBlock) || !space.pointFeature_empty(selectedBlock))
						m_window.getArea()->m_hasDigDesignations.designate(m_window.getFaction(), selectedBlock, nullptr);
			}
		}
		hide();
	});
	digButton->onMouseEnter([this, &space, &area, point]{
		auto& subMenu = makeSubmenu(0);
		auto pointFeatureTypeUI = widgetUtil::makePointFeatureTypeSelectUI();
		subMenu.add(pointFeatureTypeUI);
		auto button = tgui::Button::create("dig out feature");
		subMenu.add(button);
		button->onClick([this, &space, &area, point]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			assert(widgetUtil::lastSelectedPointFeatureType);
			const PointFeatureType& featureType = *widgetUtil::lastSelectedPointFeatureType;
			auto& selectedBlocks = m_window.getSelectedBlocks();
			if(selectedBlocks.empty())
				selectedBlocks.add(space, point);
			if(m_window.m_editMode)
			{
				for(const Cuboid& cuboid : selectedBlocks.getCuboids())
					for(const Point3D& point : cuboid.getView(space))
						if(space.solid_is(point))
							space.pointFeature_hew(point, featureType);
			}
			else
			{
				for(const Cuboid& cuboid : selectedBlocks.getCuboids())
					for(const Point3D& point : cuboid.getView(space))
						if(space.solid_is(point))
							m_window.getArea()->m_hasDigDesignations.designate(m_window.getFaction(), point, &featureType);
			}
			hide();
		});
	});
}
