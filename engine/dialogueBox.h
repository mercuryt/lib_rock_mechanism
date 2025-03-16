#pragma once

#include "index.h"
#include "types.h"

#include <string>
#include <functional>
#include <list>
#include <map>

struct DialogueBox final
{
	std::string content;
	std::map<std::string, std::function<void()>> options;
	BlockIndex location;
};

class DialogueBoxQueue final
{
	std::list<DialogueBox> m_data;
public:
	void createDialogueBox(std::string content, std::map<std::string, std::function<void()>> options, const BlockIndex location = BlockIndex::null());
	void createMessageBox(std::string content, const BlockIndex location = BlockIndex::null());
	void pop();
	[[nodiscard]] bool empty();
	[[nodiscard]] DialogueBox& top();
};
