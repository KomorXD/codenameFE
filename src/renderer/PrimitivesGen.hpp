#pragma once

#include <glm/glm.hpp>
#include <vector>

struct CubeVertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TextureUV;
};

std::vector<CubeVertex> CubeVertexData();