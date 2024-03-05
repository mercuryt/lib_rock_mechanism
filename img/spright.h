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
		data.try_emplace("roughFloor", 192, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("roughWall", 0, 149, 32, 19, 16.0, 9.5 );
		data.try_emplace("roughWallTop", 32, 168, 32, 10, 16.0, 5.0 );
		data.try_emplace("door", 160, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("flap", 128, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("hatch", 96, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("stairs", 64, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("ramp", 32, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("floorGrate", 120, 147, 32, 21, 16.0, 10.5 );
		data.try_emplace("floodGate", 0, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("fortification", 0, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("grass", 224, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bush", 192, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree1", 160, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree2", 128, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockFloor", 96, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockWall", 0, 168, 32, 17, 16.0, 8.5 );
		data.try_emplace("blockWallTop", 64, 168, 32, 6, 16.0, 3.0 );
		data.try_emplace("sleep", 181, 149, 20, 14, 10.0, 7.0 );
		data.try_emplace("hand", 60, 123, 28, 26, 14.0, 13.0 );
		data.try_emplace("open", 243, 148, 7, 9, 3.5, 4.5 );
		data.try_emplace("fluidSurface", 32, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bear", 134, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("fish", 88, 148, 32, 20, 16.0, 10.0 );
		data.try_emplace("deer", 166, 64, 32, 30, 16.0, 15.0 );
		data.try_emplace("equine", 45, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("raptor", 106, 64, 28, 31, 14.0, 15.5 );
		data.try_emplace("rabbit", 224, 32, 31, 32, 15.5, 16.0 );
		data.try_emplace("fox", 32, 123, 28, 26, 14.0, 13.0 );
		data.try_emplace("goblin", 0, 64, 23, 32, 11.5, 16.0 );
		data.try_emplace("dwarf", 77, 64, 29, 31, 14.5, 15.5 );
		data.try_emplace("troll", 228, 64, 28, 30, 14.0, 15.0 );
		data.try_emplace("axe", 0, 123, 32, 26, 16.0, 13.0 );
		data.try_emplace("fruit", 23, 64, 22, 32, 11.0, 16.0 );
		data.try_emplace("bar", 74, 95, 14, 28, 7.0, 14.0 );
		data.try_emplace("block", 88, 122, 32, 26, 16.0, 13.0 );
		data.try_emplace("board", 152, 124, 30, 23, 15.0, 11.5 );
		data.try_emplace("rock", 64, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bucket", 134, 94, 30, 30, 15.0, 15.0 );
		data.try_emplace("tool", 198, 64, 30, 30, 15.0, 15.0 );
		data.try_emplace("cart", 120, 124, 32, 23, 16.0, 11.5 );
		data.try_emplace("arrow", 211, 148, 32, 20, 16.0, 10.0 );
		data.try_emplace("crossbow", 152, 147, 29, 21, 14.5, 10.5 );
		data.try_emplace("dagger", 239, 123, 16, 25, 8.0, 12.5 );
		data.try_emplace("pile", 32, 149, 28, 19, 14.0, 9.5 );
		data.try_emplace("log", 190, 123, 21, 26, 10.5, 13.0 );
		data.try_emplace("sword", 220, 94, 29, 29, 14.5, 14.5 );
		data.try_emplace("panniers", 190, 94, 30, 29, 15.0, 14.5 );
		data.try_emplace("cloths", 88, 95, 30, 27, 15.0, 13.5 );
		data.try_emplace("peg", 60, 149, 19, 17, 9.5, 8.5 );
		data.try_emplace("meal", 211, 123, 28, 25, 14.0, 12.5 );
		data.try_emplace("rope", 164, 94, 26, 30, 13.0, 15.0 );
		data.try_emplace("seed", 120, 95, 11, 24, 5.5, 12.0 );
		data.try_emplace("pick", 45, 95, 29, 28, 14.5, 14.0 );
		data.try_emplace("mallet", 0, 96, 27, 27, 13.5, 13.5 );
		
	}
} // namespace
