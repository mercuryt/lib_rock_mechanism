#include "drama.h"
#include "config.h"
#include <fstream>
void Config::Drama::load()
{
	std::ifstream f("data/config/drama.json");
	Json data = Json::parse(f);
	data["percentWearToAddForMistakeAtWork"].get_to(percentWearToAddForMistakeAtWork);
	data["bonusPercentToAddToProjectOnSucessAtWorkSavedTime"].get_to(bonusPercentToAddToProjectOnSucessAtWorkSavedTime);
}