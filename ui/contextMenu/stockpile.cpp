#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
void ContextMenu::drawStockPileControls(BlockIndex& block)
{
	// Stockpile
	if(m_window.getFaction() && block.m_isPartOfStockPiles.contains(*m_window.getFaction()))
	{
		auto stockpileLabel = tgui::Label::create("stockpile");
		m_root.add(stockpileLabel);
		stockpileLabel->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		stockpileLabel->onMouseEnter([this, &block]{
			auto& submenu = makeSubmenu(0);
			auto shrinkButton = tgui::Button::create("shrink");
			shrinkButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(shrinkButton);
			shrinkButton->onClick([this, &block]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				StockPile* stockpile = block.m_isPartOfStockPiles.getForFaction(*m_window.getFaction());
				if(stockpile)
					for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
						if(stockpile->contains(*selectedBlock))
							stockpile->removeBlock(*selectedBlock);
				hide();
			});
			auto editButton = tgui::Button::create("edit");
			editButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(editButton);
			editButton->onClick([this, &block]{
				StockPile* stockpile = block.m_isPartOfStockPiles.getForFaction(*m_window.getFaction());
				if(stockpile)
					m_window.showEditStockPile(stockpile);
				hide();
			});
		});
	}
	else if(m_window.getFaction() && block.m_hasShapes.canStandIn())
	{
		auto createButton = tgui::Button::create("create stockpile");	
		createButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(createButton);
		createButton->onClick([this]{
			if(!m_window.getArea()->m_hasStocks.contains(*m_window.getFaction()))
				m_window.getArea()->m_hasStocks.addFaction(*m_window.getFaction());
			m_window.showEditStockPile();
			hide();
		});
	}
}
