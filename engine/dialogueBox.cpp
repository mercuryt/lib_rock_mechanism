#include "dialogueBox.h"
#include "objective.h"

void DialogueBoxQueue::createDialogueBox(std::wstring content, std::map<std::wstring, std::function<void()>> options, BlockIndex location)
{
	m_data.emplace_back(content, options, location);
}
void DialogueBoxQueue::createMessageBox(std::wstring content, BlockIndex location)
{
	std::map<std::wstring, std::function<void()>> options;
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
