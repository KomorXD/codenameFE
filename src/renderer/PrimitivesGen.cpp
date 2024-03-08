#include "PrimitivesGen.hpp"

static void CalculateTangents(Vertex& v0, Vertex& v1, Vertex& v2)
{
	glm::vec3 edge1 = v1.Position - v0.Position;
	glm::vec3 edge2 = v2.Position - v0.Position;

	glm::vec2 deltaUV1 = v1.TextureUV - v0.TextureUV;
	glm::vec2 deltaUV2 = v2.TextureUV - v0.TextureUV;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
	v0.Tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
	v0.Bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

	glm::vec3 normal = glm::normalize(v0.Normal);
	v0.Tangent = glm::normalize(v0.Tangent - normal * glm::dot(normal, v0.Tangent));
	v0.Bitangent = glm::normalize(v0.Bitangent - normal * glm::dot(normal, v0.Bitangent));
	
	v1.Tangent = v0.Tangent;
	v1.Bitangent = v0.Bitangent;
	v2.Tangent = v0.Tangent;
	v2.Bitangent = v0.Bitangent;
}

VertexData QuadMeshData()
{
    std::vector<Vertex> vertices =
    {
        {{ -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-left
        {{  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottom-right
        {{  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 1.0f, 1.0f }}, // Top-right
        {{ -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {}, {}, { 0.0f, 1.0f }}  // Top-left
    };

    std::vector<uint32_t> indices =
    {
        0, 1, 2, 2, 3, 0
    };

	CalculateTangents(vertices[0], vertices[1], vertices[2]);
	CalculateTangents(vertices[0], vertices[2], vertices[3]);

    return { vertices, indices };
}

VertexData CubeMeshData()
{
    std::vector<Vertex> vertices =
    {
        // Front
        {{ -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-left
        {{  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottom-right
        {{  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, {}, {}, { 1.0f, 1.0f }}, // Top-right
        {{ -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, {}, {}, { 0.0f, 1.0f }}, // Top-left

        // Back
        {{  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-right
        {{ -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottom-left
        {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, {}, {}, { 1.0f, 1.0f }}, // Top-left
		{{  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, {}, {}, { 0.0f, 1.0f }}, // Top-right

        // Top
		{{ -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-left
        {{  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottomm-right
        {{  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, {}, {}, { 1.0f, 1.0f }}, // Top-right
        {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, {}, {}, { 0.0f, 1.0f }}, // Top-left

        // Bottom
        {{  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottomm-right
        {{ -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-left
        {{ -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, {}, {}, { 0.0f, 1.0f }}, // Top-left
        {{  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, {}, {}, { 1.0f, 1.0f }}, // Top-right

        // Left
        {{ -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-left
        {{ -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottom-right
        {{ -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, {}, {}, { 1.0f, 1.0f }}, // Top-right
        {{ -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, {}, {}, { 0.0f, 1.0f }}, // Top-left

        // Right
        {{  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, {}, {}, { 1.0f, 0.0f }}, // Bottom-right
        {{  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, {}, {}, { 0.0f, 0.0f }}, // Bottom-left
        {{  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, {}, {}, { 0.0f, 1.0f }}, // Top-left
        {{  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, {}, {}, { 1.0f, 1.0f }}  // Top-right
    };

    std::vector<uint32_t> indices =
    {
        // Front
        0, 1, 2, 2, 3, 0,

        // Back
        4, 5, 6, 6, 7, 4,

        // Top
        8, 9, 10, 10, 11, 8,

        // Bottom
        12, 13, 14, 14, 15, 12,

        // Left
        16, 17, 18, 18, 19, 16,

        // Right
        20, 21, 22, 22, 23, 20
    };

	for (size_t i = 0; i < indices.size(); i += 6)
	{
		CalculateTangents(vertices[indices[i + 0]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
		CalculateTangents(vertices[indices[i + 3]], vertices[indices[i + 4]], vertices[indices[i + 5]]);
	}

    return { vertices, indices };
}
