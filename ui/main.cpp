#include "displayData.h"
#include "sprite.h"
#include "window.h"
#include "../engine/config.h"
#include "../engine/definitions/definitions.h"
int main()
{
	Config::load();
	definitions::load();
	ObjectiveType::load();
	displayData::load();
	sprites::load();
	tgui::Theme::setDefault("data/display/themes/Black.txt");
	Window window;
	window.startLoop();
	window.showMainMenu();
}
