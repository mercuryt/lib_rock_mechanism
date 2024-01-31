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
		data.try_emplace("roughWall", 64, 64, 32, 19, 16.0, 9.5 );
		data.try_emplace("roughWallTop", 96, 81, 32, 10, 16.0, 5.0 );
		data.try_emplace("door", 32, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("flap", 64, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("hatch", 96, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("stairs", 0, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("ramp", 32, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("floorGrate", 32, 64, 32, 21, 16.0, 10.5 );
		data.try_emplace("floodGate", 64, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("fortification", 96, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockFloor", 0, 64, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockWall", 96, 64, 32, 17, 16.0, 8.5 );
		data.try_emplace("blockWallTop", 64, 83, 32, 6, 16.0, 3.0 );
		
	}
} // namespace
