#pragma once

#include "OpenGL.hpp"
#include <unordered_map>

struct Mesh
{
	std::string Name;
	std::shared_ptr<VertexArray> VAO;
	std::shared_ptr<VertexBuffer> VBO;
	std::shared_ptr<VertexBuffer> InstanceBuffer;

	bool operator== (const Mesh& rhs) { return VAO == rhs.VAO && VBO == rhs.VBO; }
};

struct Material
{
	std::string Name;
	glm::vec4 Color = glm::vec4(1.0f);
	glm::vec2 TilingFactor = glm::vec2(1.0f);
	glm::vec2 TextureOffset = glm::vec2(0.0f);
	float Shininess = 32.0f;
	int32_t AlbedoTextureID = 1;
	int32_t NormalTextureID = 2;

	Material() = default;
	Material(const Material& other) = default;
};

class AssetManager
{
public:
	static int32_t AddMesh(Mesh& mesh);
	static int32_t AddMesh(Mesh& mesh, int32_t id);

	static const std::unordered_map<int32_t, Mesh>& AllMeshes();
	static Mesh& GetMesh(int32_t id);
	static int32_t MeshID(Mesh& mesh);
	
	static bool RemoveMesh(int32_t id);
	static bool RemoveMesh(Mesh& mesh);

	static constexpr int32_t MESH_PLANE	 = 1;
	static constexpr int32_t MESH_CUBE	 = 2;
	static constexpr int32_t MESH_SPHERE = 3;


	static int32_t AddTexture(std::shared_ptr<Texture> texture);
	static int32_t AddTexture(std::shared_ptr<Texture> texture, int32_t id);

	static const std::unordered_map<int32_t, std::shared_ptr<Texture>>& AllTextures();
	static std::shared_ptr<Texture> GetTexture(int32_t id);
	static int32_t TextureID(std::shared_ptr<Texture> texture);

	static bool RemoveTexture(int32_t id);
	static bool RemoveTexture(std::shared_ptr<Texture> texture);

	static constexpr int32_t TEXTURE_WHITE  = 1;
	static constexpr int32_t TEXTURE_NORMAL = 2;


	static int32_t AddMaterial(Material& material);
	static int32_t AddMaterial(Material& material, int32_t id);

	static const std::unordered_map<int32_t, Material>& AllMaterials();
	static Material& GetMaterial(int32_t id);
	static int32_t MaterialID(Material& material);

	static bool RemoveMaterial(int32_t id);
	static bool RemoveMaterial(Material& texture);

	static constexpr int32_t MATERIAL_DEFAULT = 1;

private:
	static std::unordered_map<int32_t, Mesh> s_Meshes;
	static int32_t s_LastMeshID;

	static std::unordered_map<int32_t, std::shared_ptr<Texture>> s_Textures;
	static int32_t s_LastTextureID;

	static std::unordered_map<int32_t, Material> s_Materials;
	static int32_t s_LastMaterialID;
};