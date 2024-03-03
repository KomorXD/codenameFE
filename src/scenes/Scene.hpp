#pragma once

#include <vector>
#include "Entities.hpp"

struct Scene
{
	std::vector<Plane>	   Planes;
	std::vector<Cube>	   Cubes;
	std::vector<LightCube> Lights;
};