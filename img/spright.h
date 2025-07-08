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
		data.try_emplace("roughFloor", 32, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("roughWall", 62, 198, 32, 19, 16.0, 9.5 );
		data.try_emplace("roughWallTop", 186, 213, 32, 10, 16.0, 5.0 );
		data.try_emplace("door", 192, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("flap", 160, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("hatch", 128, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("stairs", 96, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("ramp", 64, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("floorGrate", 30, 178, 32, 21, 16.0, 10.5 );
		data.try_emplace("floodGate", 0, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("fortification", 224, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("grass", 192, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("bush", 160, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree1", 128, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("tree2", 96, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockFloor", 64, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("blockWall", 30, 199, 32, 17, 16.0, 8.5 );
		data.try_emplace("blockWallTop", 0, 216, 32, 6, 16.0, 3.0 );
		data.try_emplace("sleep", 166, 200, 20, 14, 10.0, 7.0 );
		data.try_emplace("hand", 0, 152, 28, 26, 14.0, 13.0 );
		data.try_emplace("open", 126, 178, 7, 9, 3.5, 4.5 );
		data.try_emplace("trunk", 83, 153, 20, 25, 10.0, 12.5 );
		data.try_emplace("trunkWithBranches", 139, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("trunkLeaves", 0, 0, 32, 32, 16.0, 16.0 );
		data.try_emplace("branch", 169, 151, 28, 26, 14.0, 13.0 );
		data.try_emplace("branchLeaves", 53, 154, 30, 24, 15.0, 12.0 );
		data.try_emplace("fluidSurface", 32, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("bear", 107, 95, 32, 30, 16.0, 15.0 );
		data.try_emplace("fish", 94, 178, 32, 20, 16.0, 10.0 );
		data.try_emplace("deer", 139, 95, 32, 30, 16.0, 15.0 );
		data.try_emplace("equine", 107, 64, 32, 31, 16.0, 15.5 );
		data.try_emplace("raptor", 200, 64, 28, 31, 14.0, 15.5 );
		data.try_emplace("rabbit", 31, 64, 31, 32, 15.5, 16.0 );
		data.try_emplace("fox", 201, 125, 29, 26, 14.5, 13.0 );
		data.try_emplace("goblin", 62, 64, 23, 32, 11.5, 16.0 );
		data.try_emplace("dwarf", 171, 64, 29, 31, 14.5, 15.5 );
		data.try_emplace("troll", 0, 96, 28, 30, 14.0, 15.0 );
		data.try_emplace("axe", 169, 125, 32, 26, 16.0, 13.0 );
		data.try_emplace("fruit", 85, 64, 22, 32, 11.0, 16.0 );
		data.try_emplace("bar", 231, 124, 14, 28, 7.0, 14.0 );
		data.try_emplace("point", 0, 126, 32, 26, 16.0, 13.0 );
		data.try_emplace("board", 207, 177, 30, 23, 15.0, 11.5 );
		data.try_emplace("rock", 224, 32, 32, 32, 16.0, 16.0 );
		data.try_emplace("bucket", 171, 95, 30, 30, 15.0, 15.0 );
		data.try_emplace("tool", 201, 95, 30, 30, 15.0, 15.0 );
		data.try_emplace("cart", 144, 177, 32, 23, 16.0, 11.5 );
		data.try_emplace("arrow", 62, 178, 32, 20, 16.0, 10.0 );
		data.try_emplace("crossbow", 176, 177, 31, 23, 15.5, 11.5 );
		data.try_emplace("dagger", 237, 177, 11, 22, 5.5, 11.0 );
		data.try_emplace("pile", 94, 198, 28, 19, 14.0, 9.5 );
		data.try_emplace("log", 112, 152, 21, 26, 10.5, 13.0 );
		data.try_emplace("sword", 54, 125, 29, 29, 14.5, 14.5 );
		data.try_emplace("panniers", 54, 96, 30, 29, 15.0, 14.5 );
		data.try_emplace("cloths", 112, 125, 30, 27, 15.0, 13.5 );
		data.try_emplace("peg", 237, 199, 19, 17, 9.5, 8.5 );
		data.try_emplace("meal", 225, 152, 28, 25, 14.0, 12.5 );
		data.try_emplace("rope", 28, 96, 26, 30, 13.0, 15.0 );
		data.try_emplace("seed", 133, 177, 11, 24, 5.5, 12.0 );
		data.try_emplace("pick", 83, 125, 29, 28, 14.5, 14.0 );
		data.try_emplace("mallet", 142, 125, 27, 27, 13.5, 13.5 );
		data.try_emplace("branch", 228, 64, 22, 31, 11.0, 15.5 );
		data.try_emplace("greaves", 197, 151, 28, 26, 14.0, 13.0 );
		data.try_emplace("chain mail shirt", 28, 152, 25, 26, 12.5, 13.0 );
		data.try_emplace("shield", 0, 178, 30, 23, 15.0, 11.5 );
		data.try_emplace("half helm", 186, 200, 28, 13, 14.0, 6.5 );
		data.try_emplace("full helm", 133, 152, 28, 25, 14.0, 12.5 );
		data.try_emplace("breast plate", 144, 200, 22, 17, 11.0, 8.5 );
		data.try_emplace("mace", 84, 96, 16, 29, 8.0, 14.5 );
		data.try_emplace("spear", 231, 95, 17, 29, 8.5, 14.5 );
		data.try_emplace("glave", 0, 64, 31, 32, 15.5, 16.0 );

	}
} // namespace
