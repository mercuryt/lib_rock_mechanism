// automatically generated with spright
// https://github.com/houmain/spright
#include <unordered_map>
#include <string>

namespace spriteCoordinates
{
	struct SpriteCoords {
		int x, y, w, h;
		float pivot_x, pivot_y;
	};
	inline std::unordered_map<std::string, SpriteCoords> data;

	inline void load()
	{
		data.try_emplace("roughFloor", 0, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("roughWall", 32, 0, 32, 32, 16.0, 16.0 );
		
	}
} // namespace
