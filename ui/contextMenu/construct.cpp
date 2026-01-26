#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
#include "../../engine/pointFeature.h"
#include "../../engine/designations.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/areaBuilderUtil.h"
void ContextMenu::drawConstructControls(const Point3D& point)
{
	const Area& area = *m_window.getArea();
	const Space& space =  area.getSpace();
	const FactionId& faction = m_window.getFaction();
	if(faction.exists() && area.m_spaceDesignations.contains(faction))
	{
		const auto& designations = area.m_spaceDesignations.getForFaction(faction);
		if(designations.check(point, SpaceDesignation::Construct))
		{
			auto cancelButton = tgui::Button::create("cancel");
			cancelButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			m_root.add(cancelButton);
			cancelButton->onClick([this, faction, &space]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				for(const Cuboid& cuboid : m_window.getSelectedBlocks())
					for(const Point3D& selectedBlock : cuboid)
						m_window.getArea()->m_hasConstructionDesignations.undesignate(faction, selectedBlock);
				hide();
			});
		}
	}
	auto constructButton = tgui::Button::create("construct");
	constructButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
	m_root.add(constructButton);
	static bool constructed = false;
	constructButton->onMouseEnter([this, point]{
		auto& subMenu = makeSubmenu(0);
		// Only list material types which have construction data, unless in edit mode.
		std::function<bool(const MaterialTypeId&)> predicate = nullptr;
		if(!m_window.m_editMode)
			predicate = [&](const MaterialTypeId& materialType)->bool{ return MaterialType::construction_getConsumed(materialType).empty(); };
		auto materialTypeSelector = widgetUtil::makeMaterialSelectUI(widgetUtil::lastSelectedConstructionMaterial, "", predicate);
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
		auto pointFeatureTypeUI = widgetUtil::makePointFeatureTypeSelectUI();
		subMenu.add(pointFeatureTypeUI);
		auto constructFeature = tgui::Button::create("construct feature");
		subMenu.add(constructFeature);
		constructFeature->onClick([this, point]{
			construct(point, constructed, widgetUtil::lastSelectedConstructionMaterial, widgetUtil::lastSelectedPointFeatureType);
		});
	});
	constructButton->onClick([this, point]{
		construct(point, constructed, widgetUtil::lastSelectedConstructionMaterial, nullptr);
	});
}
void ContextMenu::construct(const Point3D& point, bool constructed, const MaterialTypeId& materialType, const PointFeatureType* pointFeatureType)
{
	std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
	CuboidSet& selectedBlocks = m_window.getSelectedBlocks();
	if(selectedBlocks.empty())
		m_window.selectBlock(point);
	Space& space = m_window.getArea()->getSpace();
	for(const Cuboid& cuboid : m_window.getSelectedBlocks().getCuboids())
	{
		if(m_window.m_editMode)
		{
			if(pointFeatureType == nullptr)
				space.solid_setCuboid(cuboid, materialType, constructed);
			else
				// TODO: Create point features by cuboid batch.
				for(const Point3D& selectedBlock : cuboid)
				{
					if(!constructed)
					{
						if(!space.solid_isAny(selectedBlock))
							space.solid_set(selectedBlock, materialType, false);
						space.pointFeature_hew(selectedBlock, PointFeatureType::getId(*pointFeatureType));
					}
					else
						space.pointFeature_construct(selectedBlock, PointFeatureType::getId(*pointFeatureType), materialType);
				}
		}
		else
			for(const Point3D& selectedBlock : cuboid)
				m_window.getArea()->m_hasConstructionDesignations.designate(m_window.getFaction(), selectedBlock, PointFeatureType::getId(*pointFeatureType), materialType);
	}
	hide();
}
