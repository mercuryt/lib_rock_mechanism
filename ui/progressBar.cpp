#include "progressBar.h"
#include "window.h"
ProgressBar::ProgressBar(Window& window) : m_window(window)
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}
void ProgressBar::draw()
{
	m_panel->removeAllWidgets();
	auto layoutGrid = tgui::Grid::create();
	uint8_t gridCount = 0;
	layoutGrid->addWidget(tgui::Label::create(m_title), ++gridCount, 1);
	layoutGrid->addWidget(tgui::Label::create(std::to_string(m_percent.get()) + "%"), ++gridCount, 1);
	m_panel->setVisible(true);
}