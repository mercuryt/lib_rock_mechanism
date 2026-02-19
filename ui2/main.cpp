#include "window.h"
#include "displayData.h"
//#include "sprite.h"
#include "../engine/config/config.h"
#include "../engine/definitions/definitions.h"
#include "../engine/objective.h"

int main ([[maybe_unused]]int argCount, [[maybe_unused]]char **args)
{
	  // Setup SDL
	#ifdef _WIN32
		::SetProcessDPIAware();
	#endif
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
		{
			printf("Error: %s\n", SDL_GetError());
			return 1;
		}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	Config::load();
	definitions::load();
	ObjectiveType::load();
	displayData::load();
	//sprites::load();
	Window window;
	window.startLoop();
	return 0;
}