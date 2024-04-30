#pragma once

#include <entt/entt.hpp>
#include <string>

#include "../renderer/Camera.hpp"

class Entity;

struct Scene
{
	Entity SpawnEntity(const std::string& name);
	void DestroyEntity(Entity entity);

	void RenderShadowMaps();
	void Render(Camera& editorCamera);

private:
	entt::registry m_Registry;
	std::vector<Entity> m_Entities;

	friend class Entity;
	friend class EditorLayer;
};