#pragma once

#include <glm/glm.hpp>
#include <vector>

struct CubeVertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
};

std::vector<CubeVertex> CubeVertexData();