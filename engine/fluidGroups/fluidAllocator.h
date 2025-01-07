#pragma once
#include <memory_resource>
using FluidAllocator = std::pmr::unsynchronized_pool_resource;