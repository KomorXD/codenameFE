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

	static Mesh& GetMesh(int32_t id);
	static int32_t MeshID(Mesh& mesh);
	
	static bool RemoveMesh(int32_t id);
	static bool RemoveMesh(Mesh& mesh);

private:
	static std::unordered_map<int32_t, Mesh> s_Meshes;
	static int32_t s_MeshesCount;
};