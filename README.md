<h3 align="center">lib_rock_mechanim</h3>

  <p align="center">
    This library impliments four algorithms for use by games like Dwarf Fortress. These four are: vision (who can see who), pathfinding (how can someone get somewhere), fluid flow (where do liquids and heavy gases go), and cave-in (does the roof collapse).
  </p>
</div>

## About The Project

Features:
* Vision:
    -    Parallelized in batches of configurable size.
    -    Multi-block creatures.
    -    Distance bonus / penalty based on target location (glow tiles).
* Pathfinding:
    -    Parallelized in batches of configurable size.
    -    Multi-block creatures.
* Fluid flow:
    -    Parallelized by groups of contiguous blocks which contain the same fluid.
    -    Fluid mixing / seperation by denisity.
* Cave-in:
    -    Nothing special.
* All of the above:
    -    Able to run their 'readStep' methods in parallel with eachother.

## Benchmarks

* Glass Rave:
	Benchmarking speed of vision with open sightlines.
	An area is created with dimensions of 200x200x5. The bottom layer is filled with solid stone, all other layers are filled with glass floor. An DerivedActor is placed in every block which is not solid, 1600 actors in total. 100 steps are processed.
	The average time for a step is 23ms and the longest step is 35ms.
* Stone Rave:
	Benchmarking speed of vision with closed sightlines.
	The same as Glass Rave except the floors are all stone rather then glass.
	The average time of a step is 22ms and the longest is 29ms.
* Four Fluids:
	An area is created with solid stone layers on the bottom and sides with non-solid dimensions of 80x80x20. Four fluid groups are created, eeach with dimensions 20x20x20. The four fluid types are water, CO2, mercury and lava. This stabilizes into four layes over the course of 80 steps.
	The average time of a step is 203ms and the longest is 301ms.
* All of the above were run on a AMD A9 processor ( 2 threads @ 2.6 ghz ) with 4 gb RAM. RAM usage is less then 100MB.

## Getting Started

Using this library will require implimenting three classes which must inherit from provided base classes: **DerivedBlock**, **DerivedArea**, and **DerivedActor** must inherit from BaseBlock,  BaseArea, and BaseActor. Each of these has one or more methods which must be defined. **DerivedBlock must be declared before DerivedArea.** For an example of method definitions which configure the library to behave much like Dwarf Fortress see the [Test Implimentation](https://github.com/mercuryt/lib_rock_mechanism/blob/master/test/testShared.h). Copying and editing that file is probably a good start twords your implimentation.

<!-- CONTRIBUTING -->
## Contributing

Any contributions are **greatly appreciated**.
