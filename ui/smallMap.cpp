/*
	Explicit template instantiations for ui.
*/
#include "../engine/dataStructures/smallMap.hpp"
#include "../engine/numericTypes/idTypes.h"
#include "displayData.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

template class SmallMap<MaterialTypeId, sf::Color>;
template class SmallMap<FluidTypeId, sf::Color>;
template class SmallMap<ItemTypeId, ItemTypeDisplayData>;
template class SmallMap<PlantSpeciesId, PlantSpeciesDisplayData>;
template class SmallMap<AnimalSpeciesId, AnimalSpeciesDisplayData>;
template class SmallMap<std::string, std::pair<sf::Texture, sf::Vector2f>>;
//template class SmallMap<sf::Texture, sf::Vector2<float>>;