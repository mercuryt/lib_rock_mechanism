#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
void ContextMenu::drawStockPileControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	const FactionId& faction = m_window.getFaction();
	// Stockpile
	if(faction.exists() && space.stockpile_contains(point, faction))
	{
		auto stockpileLabel = tgui::Label::create("stockpile");
		m_root.add(stockpileLabel);
		stockpileLabel->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		stockpileLabel->onMouseEnter([this, point, faction, &space]{
			auto& submenu = makeSubmenu(0);
			auto shrinkButton = tgui::Button::create("shrink");
			shrinkButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(shrinkButton);
			shrinkButton->onClick([this, point, faction, &space]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				StockPile* stockpile = space.stockpile_getOneForFaction(point, faction);
				if(stockpile)
					for(const Cuboid& cuboid : m_window.getSelectedBlocks())
						for(const Point3D& selectedBlock : cuboid)
							if(stockpile->contains(selectedBlock))
								stockpile->removePoint(selectedBlock);
				hide();
			});
			auto editButton = tgui::Button::create("edit");
			editButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(editButton);
			editButton->onClick([this, point, faction, &space]{
				StockPile* stockpile = space.stockpile_getOneForFaction(point, faction);
				if(stockpile)
					m_window.showEditStockPile(stockpile);
				hide();
			});
		});
	}
	else if(faction.exists() && space.shape_canStandIn(point))
	{
		auto createButton = tgui::Button::create("create stockpile");
		createButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(createButton);
		createButton->onClick([this, faction]{
			if(!m_window.getArea()->m_hasStocks.contains(faction))
				m_window.getArea()->m_hasStocks.addFaction(faction);
			m_window.showEditStockPile();
			hide();
		});
	}
}
