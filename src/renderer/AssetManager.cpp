#include "AssetManager.hpp"

std::unordered_map<int32_t, Mesh> AssetManager::s_Meshes;
int32_t AssetManager::s_LastMeshID = 0;

int32_t AssetManager::AddMesh(Mesh& mesh)
{
	for (const auto& [key, val] : s_Meshes)
	{
		if (val == mesh)
		{
			return key;
		}
	}

	s_LastMeshID++;
	s_Meshes.insert({ s_LastMeshID, mesh });
	return s_LastMeshID;
}

int32_t AssetManager::AddMesh(Mesh& mesh, int32_t id)
{
	ASSERT(!s_Meshes.contains(id) && "Cannot override existing mesh");

	for (const auto& [key, val] : s_Meshes)
	{
		if (val == mesh)
		{
			return key;
		}
	}

	s_LastMeshID = id;
	s_Meshes.insert({ id, mesh });
	return s_LastMeshID;
}

Mesh& AssetManager::GetMesh(int32_t id)
{
	ASSERT(s_Meshes.contains(id) && "Trying to access non-existing mesh");
	return s_Meshes[id];
}

int32_t AssetManager::MeshID(Mesh& mesh)
{
	for (const auto& [key, val] : s_Meshes)
	{
		if (val == mesh)
		{
			return key;
		}
	}

	ASSERT(false && "Mesh doesn't exist");
}

bool AssetManager::RemoveMesh(int32_t id)
{
	if (!s_Meshes.contains(id))
	{
		return false;
	}

	s_Meshes.erase(id);
	return true;
}

bool AssetManager::RemoveMesh(Mesh& mesh)
{
	int32_t id = MeshID(mesh);
	return RemoveMesh(id);
}
