#include "dialogueBox.h"
#include "window.h"
#include "../engine/simulation.h"
#include "../engine/dialogueBox.h"
#include <TGUI/Widgets/HorizontalLayout.hpp>
bool DialogueBoxUI::empty() const
{
	return m_window.getSimulation()->m_hasDialogues.empty();
}

void DialogueBoxUI::draw()
{
	assert(!empty());
	DialogueBox& dialogueBox = m_window.getSimulation()->m_hasDialogues.top();
	m_childWindow = tgui::ChildWindow::create();
	auto content = tgui::Label::create(dialogueBox.content);
	content->setPosition("5%", "5%");
	m_childWindow->add(content);
	auto buttonHolder = tgui::HorizontalLayout::create();
	m_childWindow->add(buttonHolder);
	buttonHolder->setPosition(tgui::bindLeft(content), tgui::bindBottom(content) + 10);
	m_window.getGui().add(m_childWindow);
	for(auto& [labelText, callback] : dialogueBox.options)
	{
		auto button = tgui::Button::create(labelText);
		button->onClick([this, callback]{
			m_window.getSimulation()->m_hasDialogues.pop();
			m_childWindow->close();
			callback();
		});
		buttonHolder->add(button);
	}
	if(dialogueBox.location != BLOCK_INDEX_MAX)
		m_window.centerView(dialogueBox.location);
}
void DialogueBoxUI::hide()
{
	m_childWindow->close();
	m_childWindow = nullptr;
	if(!empty())
		draw();
}
bool DialogueBoxUI::isVisible() const
{
	return m_childWindow != nullptr;
}
