#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
void ContextMenu::drawStockPileControls(const BlockIndex& block)
{
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	const FactionId& faction = m_window.getFaction();
	// Stockpile
	if(faction.exists() && blocks.stockpile_contains(block, faction))
	{
		auto stockpileLabel = tgui::Label::create("stockpile");
		m_root.add(stockpileLabel);
		stockpileLabel->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		stockpileLabel->onMouseEnter([this, block, faction, &blocks]{
			auto& submenu = makeSubmenu(0);
			auto shrinkButton = tgui::Button::create("shrink");
			shrinkButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(shrinkButton);
			shrinkButton->onClick([this, block, faction, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				StockPile* stockpile = blocks.stockpile_getForFaction(block, faction);
				if(stockpile)
					for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
						if(stockpile->contains(selectedBlock))
							stockpile->removeBlock(selectedBlock);
				hide();
			});
			auto editButton = tgui::Button::create("edit");
			editButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(editButton);
			editButton->onClick([this, block, faction, &blocks]{
				StockPile* stockpile = blocks.stockpile_getForFaction(block, faction);
				if(stockpile)
					m_window.showEditStockPile(stockpile);
				hide();
			});
		});
	}
	else if(faction.exists() && blocks.shape_canStandIn(block))
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
