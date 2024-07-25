#pragma once

#include "index.h"
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
	void createDialogueBox(std::wstring content, std::map<std::wstring, std::function<void()>> options, BlockIndex location = BlockIndex::null());
	void createMessageBox(std::wstring content, BlockIndex location = BlockIndex::null());
	void pop();
	[[nodiscard]] bool empty();
	[[nodiscard]] DialogueBox& top();
};
