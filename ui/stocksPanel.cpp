#include "stocksPanel.h"
#include "window.h"
#include "../engine/items/items.h"
#include <string>
struct MaterialType;
StocksView::StocksView(Window& window) : m_window(window), m_group(tgui::Group::create()), m_list(tgui::ScrollablePanel::create())
{
	m_window.getGui().add(m_group);
	m_group->setVisible(false);
	m_group->add(tgui::Label::create("Stocks"));
	m_group->add(m_list);
	auto exit = tgui::Button::create("x");
	m_group->add(exit);
	exit->onClick([&]{ m_window.showGame(); });
	auto create = tgui::Button::create("+");
	m_group->add(create);
	exit->onClick([&]{ m_window.showProduction(); });
}
void StocksView::draw()
{
	for(const auto& [itemType, map] : m_window.getArea()->m_hasStocks.getForFaction(m_window.getFaction()).get())
	{
		for(const auto& [materialType, items] : map)
		{
			const uint32_t count = items.size();
			std::wstring describe(MaterialType::getName(materialType) + L" " + ItemType::getName(itemType) + L"(" + std::to_wstring(count) + L")");
			m_list->add(tgui::Label::create(describe));
		}
	}
}
