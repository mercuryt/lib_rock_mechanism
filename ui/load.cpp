#include "load.h"
#include "window.h"
#include <filesystem>
void LoadView::draw()
{
	m_panel = tgui::Panel::create();
	m_window.getGui().add(m_panel);
	auto layout = tgui::VerticalLayout::create();
	m_panel->add(layout);
	layout->add(tgui::Label::create("load"));
	auto cancel = tgui::Button::create("cancel");
	layout->add(cancel);
	cancel->onClick([this]{ m_window.showMainMenu(); });
	std::filesystem::path save{"save"};
	for(const auto &entry : std::filesystem::directory_iterator(save))
	{
		auto entryPath = entry.path();
		std::ifstream simulationFile(entryPath/"simulation.json");
		const Json& data = Json::parse(simulationFile);
		std::wstring name = data["name"].get<std::wstring>();
		auto button = tgui::Button::create(name);
		layout->add(button);
		button->onClick([this, entryPath]{ m_window.load(entryPath); });
	}
}
void LoadView::close()
{
	if(m_panel)
	{
		m_window.getGui().remove(m_panel);
		m_panel = nullptr;
	}
}
