#pragma once
#include "../../engine/numericTypes/idTypes.h"
#include "../../engine/numericTypes/types.h"
#include "../../engine/config/config.h"
#include "../../engine/geometry/point3D.h"
class Window;
namespace contextMenu
{
	void draw(Window& window);
	namespace controlls
	{
		void dig(Window& window);
		void construct(Window& window);
		void fluid(Window& window);
		void plants(Window& window);
		void items(Window& window);
		void woodcutting(Window& window);
		void actors(Window& window);
	}
	namespace helpers
	{
		void construct(Window& window, bool feature);
	}
}