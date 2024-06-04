#pragma once

#include "types.h"

#include <string>
#include <functional>
#include <list>
#include <map>

struct DialogueBox final
{
	std::wstring content;
	std::map<std::wstring, std::function<void()>> options;
	BlockIndex location;
};

class DialogueBoxQueue final
{
	std::list<DialogueBox> m_data;
public:
	void createDialogueBox(std::wstring content, std::map<std::wstring, std::function<void()>> options, BlockIndex location = BLOCK_INDEX_MAX);
	void createMessageBox(std::wstring content, BlockIndex location = BLOCK_INDEX_MAX);
	void pop();
	[[nodiscard]] bool empty();
	[[nodiscard]] DialogueBox& top();
};
