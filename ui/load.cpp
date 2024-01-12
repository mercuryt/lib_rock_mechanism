#include "load.h"
#include "window.h"
class Window;
LoadView::LoadView(Window& window) : m_window(window), m_group(tgui::Group::create())
{
	m_window.m_gui.add(m_group);
	// File Select.
	auto fileDialog = tgui::FileDialog::create("open file", "open");
	fileDialog->onFileSelect([&](const std::filesystem::path path){ m_window.load(path); });
	fileDialog->setPath("save");
	m_group->add(fileDialog);
	// Cancel.
	auto cancelButton = tgui::Button::create("cancel");
	cancelButton->onPress([&]{ m_window.showMainMenu(); });
	m_group->add(cancelButton);
}
