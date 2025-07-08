#include "dialogueBox.h"
#include "objective.h"

void DialogueBoxQueue::createDialogueBox(std::string content, std::map<std::string, std::function<void()>> options, Point3D location)
{
	m_data.emplace_back(content, options, location);
}
void DialogueBoxQueue::createMessageBox(std::string content, Point3D location)
{
	std::map<std::string, std::function<void()>> options;
	m_data.emplace_back(content, options, location);
}
void DialogueBoxQueue::pop()
{
	m_data.pop_front();
}
bool DialogueBoxQueue::empty()
{
	return m_data.empty();
}
DialogueBox& DialogueBoxQueue::top()
{
	return m_data.front();
}
