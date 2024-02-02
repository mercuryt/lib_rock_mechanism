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
		data.try_emplace("roughWall", 0, 144, 32, 19, 16.0, 9.5 );
		data.try_emplace("roughWallTop", 92, 144, 32, 10, 16.0, 5.0 );
		data.try_emplace("door", 128, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("flap", 96, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("hatch", 64, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("stairs", 32, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("ramp", 0, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("floorGrate", 115, 123, 32, 21, 16.0, 10.5 );
		data.try_emplace("floodGate", 224, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("fortification", 192, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("grass", 160, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bush", 128, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree", 96, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockFloor", 64, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockWall", 60, 144, 32, 17, 16.0, 8.5 );
		data.try_emplace("blockWallTop", 179, 145, 32, 6, 16.0, 3.0 );
		data.try_emplace("bear", 89, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("fish", 147, 142, 32, 20, 16.0, 10.0 );
		data.try_emplace("deer", 121, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("equine", 0, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("raptor", 61, 64, 28, 31, 14.0, 15.5 );
		data.try_emplace("rabbit", 160, 32, 31, 32, 15.5, 16.0 );
		data.try_emplace("fox", 32, 95, 28, 26, 14.0, 13.0 );
		data.try_emplace("goblin", 191, 32, 23, 32, 11.5, 16.0 );
		data.try_emplace("dwarf", 32, 64, 29, 31, 14.5, 15.5 );
		data.try_emplace("troll", 213, 64, 28, 30, 14.0, 15.0 );
		data.try_emplace("axe", 204, 94, 32, 26, 16.0, 13.0 );
		data.try_emplace("fruit", 214, 32, 22, 32, 11.0, 16.0 );
		data.try_emplace("bar", 241, 32, 14, 28, 7.0, 14.0 );
		data.try_emplace("block", 0, 95, 32, 26, 16.0, 13.0 );
		data.try_emplace("board", 32, 121, 30, 23, 15.0, 11.5 );
		data.try_emplace("rock", 32, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bucket", 183, 64, 30, 30, 15.0, 15.0 );
		data.try_emplace("tool", 153, 64, 30, 30, 15.0, 15.0 );
		data.try_emplace("cart", 0, 121, 32, 23, 16.0, 11.5 );
		data.try_emplace("arrow", 62, 124, 32, 20, 16.0, 10.0 );
		data.try_emplace("crossbow", 174, 121, 29, 21, 14.5, 10.5 );
		data.try_emplace("dagger", 236, 94, 16, 25, 8.0, 12.5 );
		data.try_emplace("pile", 32, 144, 28, 19, 14.0, 9.5 );
		data.try_emplace("log", 60, 95, 21, 26, 10.5, 13.0 );
		data.try_emplace("sword", 145, 94, 29, 29, 14.5, 14.5 );
		data.try_emplace("panniers", 115, 94, 30, 29, 15.0, 14.5 );
		data.try_emplace("cloths", 174, 94, 30, 27, 15.0, 13.5 );
		data.try_emplace("peg", 94, 124, 19, 17, 9.5, 8.5 );
		data.try_emplace("meal", 204, 120, 28, 25, 14.0, 12.5 );
		data.try_emplace("rope", 89, 94, 26, 30, 13.0, 15.0 );
		data.try_emplace("seed", 236, 119, 11, 24, 5.5, 12.0 );
		
	}
} // namespace
