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
		data.try_emplace("roughWall", 195, 144, 32, 19, 16.0, 9.5 );
		data.try_emplace("roughWallTop", 147, 146, 32, 10, 16.0, 5.0 );
		data.try_emplace("door", 128, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("flap", 96, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("hatch", 64, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("stairs", 32, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("ramp", 0, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("floorGrate", 83, 124, 32, 21, 16.0, 10.5 );
		data.try_emplace("floodGate", 224, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("fortification", 192, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("grass", 160, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bush", 128, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree1", 96, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree2", 64, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockFloor", 32, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockWall", 72, 145, 32, 17, 16.0, 8.5 );
		data.try_emplace("blockWallTop", 147, 156, 32, 6, 16.0, 3.0 );
		data.try_emplace("bear", 111, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("fish", 115, 144, 32, 20, 16.0, 10.0 );
		data.try_emplace("deer", 143, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("equine", 22, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("raptor", 83, 64, 28, 31, 14.0, 15.5 );
		data.try_emplace("rabbit", 192, 32, 31, 32, 15.5, 16.0 );
		data.try_emplace("fox", 0, 121, 28, 26, 14.0, 13.0 );
		data.try_emplace("goblin", 223, 32, 23, 32, 11.5, 16.0 );
		data.try_emplace("dwarf", 54, 64, 29, 31, 14.5, 15.5 );
		data.try_emplace("troll", 111, 94, 28, 30, 14.0, 15.0 );
		data.try_emplace("axe", 22, 95, 32, 26, 16.0, 13.0 );
		data.try_emplace("fruit", 0, 64, 22, 32, 11.0, 16.0 );
		data.try_emplace("bar", 235, 64, 14, 28, 7.0, 14.0 );
		data.try_emplace("block", 54, 95, 32, 26, 16.0, 13.0 );
		data.try_emplace("board", 165, 123, 30, 23, 15.0, 11.5 );
		data.try_emplace("rock", 160, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("bucket", 175, 64, 30, 30, 15.0, 15.0 );
		data.try_emplace("tool", 205, 64, 30, 30, 15.0, 15.0 );
		data.try_emplace("cart", 224, 121, 32, 23, 16.0, 11.5 );
		data.try_emplace("arrow", 115, 124, 32, 20, 16.0, 10.0 );
		data.try_emplace("crossbow", 195, 123, 29, 21, 14.5, 10.5 );
		data.try_emplace("dagger", 56, 121, 16, 25, 8.0, 12.5 );
		data.try_emplace("pile", 227, 144, 28, 19, 14.0, 9.5 );
		data.try_emplace("log", 86, 95, 21, 26, 10.5, 13.0 );
		data.try_emplace("sword", 195, 94, 29, 29, 14.5, 14.5 );
		data.try_emplace("panniers", 165, 94, 30, 29, 15.0, 14.5 );
		data.try_emplace("cloths", 224, 94, 30, 27, 15.0, 13.5 );
		data.try_emplace("peg", 28, 146, 19, 17, 9.5, 8.5 );
		data.try_emplace("meal", 28, 121, 28, 25, 14.0, 12.5 );
		data.try_emplace("rope", 139, 94, 26, 30, 13.0, 15.0 );
		data.try_emplace("seed", 72, 121, 11, 24, 5.5, 12.0 );
		
	}
} // namespace
