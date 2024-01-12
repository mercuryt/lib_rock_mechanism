#include "window.h"
#include "../engine/config.h"
#include "../engine/definitions.h"
#include "../engine/objectiveTypeInstances.h"
int main()
{
	Config::load();
	definitions::load();
	ObjectiveTypeInstances::load();
	Window window;
	window.showMainMenu();
}
