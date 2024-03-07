#pragma once

#include <unordered_map>
#include "OpenGL.hpp"

struct Mesh
{
	std::string MeshName;
	std::shared_ptr<VertexArray> VAO;
	std::shared_ptr<VertexBuffer> VBO;
	std::shared_ptr<VertexBuffer> InstanceBuffer;

	bool operator== (const Mesh& rhs) { return VAO == rhs.VAO && VBO == rhs.VBO; }
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

	static constexpr int32_t MESH_PLANE = 1;
	static constexpr int32_t MESH_CUBE  = 2;


	static int32_t AddTexture(std::shared_ptr<Texture> texture);
	static int32_t AddTexture(std::shared_ptr<Texture> texture, int32_t id);

	static const std::unordered_map<int32_t, std::shared_ptr<Texture>>& AllTextures();
	static std::shared_ptr<Texture> GetTexture(int32_t id);
	static int32_t TextureID(std::shared_ptr<Texture> texture);

	static bool RemoveTexture(int32_t id);
	static bool RemoveTexture(std::shared_ptr<Texture> texture);

	static constexpr int32_t TEXTURE_WHITE = 1;

private:
	static std::unordered_map<int32_t, Mesh> s_Meshes;
	static int32_t s_LastMeshID;

	static std::unordered_map<int32_t, std::shared_ptr<Texture>> s_Textures;
	static int32_t s_LastTextureID;
};