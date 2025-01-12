#include "editDramaPanel.h"
#include "window.h"
#include "drama/engine.h"

EditDramaView::EditDramaView(Window& window) : m_window(window), m_panel(tgui::Panel::create())
{
	m_window.getGui().add(m_panel);
	hide();
}
void EditDramaView::draw(Area* area)
{
	assert(area);
	DramaEngine* dramaEngine = m_window.getSimulation()->m_dramaEngine.get();
	std::vector<DramaArc*> arcsForArea = dramaEngine->getArcsForArea(*area);
	auto listHolder = tgui::Grid::create();
	m_panel->add(listHolder);
	uint16_t i = 0;
	std::unordered_set<DramaArcType> typesForArea;
	for(DramaArc* arc : arcsForArea)
	{
		listHolder->addWidget(tgui::Label::create(arc->name()), i, 0);
		auto start = tgui::Button::create("start");
		listHolder->addWidget(start, i, 1);
		start->onClick([this, arc]{
			arc->begin();
			m_window.showGame();
		});
		auto remove = tgui::Button::create("remove");
		listHolder->addWidget(remove, i++, 2);
		remove->onClick([this, area, arc, dramaEngine]{
			dramaEngine->remove(*arc);
			m_window.showEditDrama(area);
		});
		typesForArea.insert(arc->m_type);
	}
	auto newTypeSelector = tgui::ComboBox::create();
	newTypeSelector->setPosition(tgui::bindLeft(listHolder), tgui::bindBottom(listHolder) + 10);
	newTypeSelector->addItem("select an arc type to link with this area", "none");
	newTypeSelector->setSelectedItemById("none");
	m_panel->add(newTypeSelector);
	for(DramaArcType type : DramaArc::getTypes())
	{
		if(!typesForArea.contains(type))
		{
			std::wstring string = DramaArc::typeToString(type);
			newTypeSelector->addItem(string, string);
		}
	}
	newTypeSelector->onItemSelect([this, area](const tgui::String& id){
		if(id == "none")
			return;
		auto& dramaEngine = m_window.getSimulation()->m_dramaEngine;
		dramaEngine->createArcTypeForArea(DramaArc::stringToType(id.toWideString()), *area);
		m_window.showEditDrama(area);
	});
	auto back = tgui::Button::create("close");
	back->onClick([this]{ m_window.showGame(); });
	back->setPosition(tgui::bindLeft(listHolder), tgui::bindBottom(newTypeSelector) + 10);
	m_panel->add(back);
	show();
}
