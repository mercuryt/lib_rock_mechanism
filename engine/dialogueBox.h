#pragma once
#include <string>
#include <functional>
#include <list>
#include <map>
class Block;
struct DialogueBox final
{
	std::wstring content;
	std::map<std::wstring, std::function<void()>> options;
	Block* location;
};

class DialogueBoxQueue final
{
	std::list<DialogueBox> m_data;
public:
	void createDialogueBox(std::wstring content, std::map<std::wstring, std::function<void()>> options, Block* location = nullptr);
	void createMessageBox(std::wstring content, Block* location = nullptr);
	void pop();
	[[nodiscard]] bool empty();
	[[nodiscard]] DialogueBox& top();
};
