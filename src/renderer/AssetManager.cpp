#include "AssetManager.hpp"
#include <cassert>

std::unordered_map<int32_t, Mesh> AssetManager::s_Meshes;
int32_t AssetManager::s_LastMeshID = 0;

std::unordered_map<int32_t, std::shared_ptr<Texture>> AssetManager::s_Textures;
int32_t AssetManager::s_LastTextureID = 0;

std::unordered_map<int32_t, Material> AssetManager::s_Materials;
int32_t AssetManager::s_LastMaterialID = 0;

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

const std::unordered_map<int32_t, Mesh>& AssetManager::AllMeshes()
{
	return s_Meshes;
}

Mesh& AssetManager::GetMesh(int32_t id)
{
	assert(s_Meshes.contains(id) && "Trying to access non-existing mesh");
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

	assert(false && "Mesh doesn't exist");
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

int32_t AssetManager::AddTexture(std::shared_ptr<Texture> texture)
{
	for (const auto& [key, val] : s_Textures)
	{
		if (val->GetID() == texture->GetID())
		{
			return key;
		}
	}

	s_LastTextureID++;
	s_Textures.insert({ s_LastTextureID, texture });
	return s_LastTextureID;
}

int32_t AssetManager::AddTexture(std::shared_ptr<Texture> texture, int32_t id)
{
	for (const auto& [key, val] : s_Textures)
	{
		if (val->GetID() == texture->GetID())
		{
			return key;
		}
	}

	s_LastTextureID = id;
	s_Textures.insert({ id, texture });
	return s_LastTextureID;
}

const std::unordered_map<int32_t, std::shared_ptr<Texture>>& AssetManager::AllTextures()
{
	return s_Textures;
}

std::shared_ptr<Texture> AssetManager::GetTexture(int32_t id)
{
	assert(s_Textures.contains(id) && "Trying to access non-existing texture");
	return s_Textures[id];
}

int32_t AssetManager::TextureID(std::shared_ptr<Texture> texture)
{
	for (const auto& [key, val] : s_Textures)
	{
		if (val->GetID() == texture->GetID())
		{
			return key;
		}
	}

	assert(false && "Texture doesn't exist");
}

bool AssetManager::RemoveTexture(int32_t id)
{
	if (!s_Textures.contains(id))
	{
		return false;
	}

	s_Textures.erase(id);
	return true;
}

bool AssetManager::RemoveTexture(std::shared_ptr<Texture> texture)
{
	int32_t id = TextureID(texture);
	return RemoveTexture(id);
}

int32_t AssetManager::AddMaterial(Material& material)
{
	for (const auto& [key, val] : s_Materials)
	{
		if (&val == &material)
		{
			return key;
		}
	}

	s_LastMaterialID++;
	s_Materials.insert({ s_LastMaterialID, material });
	return s_LastMaterialID;
}

int32_t AssetManager::AddMaterial(Material& material, int32_t id)
{
	for (const auto& [key, val] : s_Materials)
	{
		if (&val == &material)
		{
			return key;
		}
	}

	s_LastMaterialID = id;
	s_Materials.insert({ id, material });
	return s_LastMaterialID;
}

const std::unordered_map<int32_t, Material>& AssetManager::AllMaterials()
{
	return s_Materials;
}

Material& AssetManager::GetMaterial(int32_t id)
{
	assert(s_Materials.contains(id) && "Trying to access non-existing material");
	return s_Materials[id];
}

int32_t AssetManager::MaterialID(Material& material)
{
	for (const auto& [key, val] : s_Materials)
	{
		if (&val == &material)
		{
			return key;
		}
	}

	assert(false && "Material doesn't exist");
}

bool AssetManager::RemoveMaterial(int32_t id)
{
	if (!s_Materials.contains(id))
	{
		return false;
	}

	s_Textures.erase(id);
	return true;
}

bool AssetManager::RemoveMaterial(Material& material)
{
	int32_t id = MaterialID(material);
	return RemoveMaterial(id);
}

