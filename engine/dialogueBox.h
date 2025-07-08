#pragma once

#include "geometry/point3D.h"

#include <string>
#include <functional>
#include <list>
#include <map>

struct DialogueBox final
{
	std::string content;
	std::map<std::string, std::function<void()>> options;
	Point3D location;
};

class DialogueBoxQueue final
{
	std::list<DialogueBox> m_data;
public:
	void createDialogueBox(std::string content, std::map<std::string, std::function<void()>> options, const Point3D location = Point3D::null());
	void createMessageBox(std::string content, const Point3D location = Point3D::null());
	void pop();
	[[nodiscard]] bool empty();
	[[nodiscard]] DialogueBox& top();
};
