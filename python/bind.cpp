#include "../src/simulation.h"
#include "../src/definitions.h"
#include "../src/config.h"
#include "../src/datetime.h"
#include "../src/types.h"

#include <nanobind/nanobind.h>
#include <cstdint>

namespace nb = nanobind;

void loadConfigAndDefinitions()
{
	Config::load();
	definitions::load();
}

NB_MODULE(bound, m)
{
	m.def("loadConfigAndDefinitions", &loadConfigAndDefinitions);
	nb::class_<DateTime>(m, "DateTime")
		.def(nb::init<uint8_t, uint16_t, uint16_t>())
		.def_rw("hour", &DateTime::hour)
		.def_rw("day", &DateTime::day)
		.def_rw("year", &DateTime::year);
	nb::class_<Simulation>(m, "Simulation")
		.def(nb::init<DateTime, Step>())
		.def("doStep", &Simulation::doStep)
		.def("createArea", &Simulation::createArea, nb::rv_policy::reference)
		.def("createActor", &Simulation::createActor, nb::rv_policy::reference)
		.def("createItemGeneric", &Simulation::createItemGeneric, nb::rv_policy::reference)
		.def("createItemNongeneric", &Simulation::createItemNongeneric, nb::rv_policy::reference);
	nb::class_<Area>(m, "Area")
		.def("getBlock", &Area::getBlock, nb::rv_policy::reference);
	nb::class_<Block>(m, "Block")
		.def("isSolid", &Block::isSolid)
		.def("getSolidMaterial", &Block::getSolidMaterial, nb::rv_policy::reference)
		.def_ro("totalFluidVolume", &Block::m_totalFluidVolume);

	nb::class_<MaterialType>(m, "MaterialType")
		.def_ro("transparent", &MaterialType::transparent);
}
