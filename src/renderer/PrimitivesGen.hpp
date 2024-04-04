#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	glm::vec2 TextureUV;
};

struct VertexData
{
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;
};

VertexData QuadMeshData();
VertexData CubeMeshData();
VertexData SphereMeshData();