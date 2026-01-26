#include "load.h"
#include "window.h"
#include <filesystem>
#include <fstream>
void LoadView::draw()
{
	m_panel = tgui::Panel::create();
	m_window.getGui().add(m_panel);
	auto layout = tgui::VerticalLayout::create();
	layout->setPosition("20%", "10%");
	layout->setSize("60%", "80%");
	m_panel->add(layout);
	auto title = tgui::Label::create("Load");
	title->setPosition("50%", "5%");
	title->setOrigin(0.5, 0);
	layout->add(title);
	auto cancel = tgui::Button::create("cancel");
	layout->add(cancel);
	cancel->onClick([this]{ m_window.showMainMenu(); });
	std::filesystem::path save{"save"};
	for(const auto &entry : std::filesystem::directory_iterator(save))
	{
		auto entryPath = entry.path();
		std::ifstream simulationFile(entryPath/"simulation.json");
		const Json& data = Json::parse(simulationFile);
		std::string name = data["name"].get<std::string>();
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
