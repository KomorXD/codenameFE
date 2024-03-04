#pragma once

#include <entt/entt.hpp>
#include <string>

#include "../renderer/Camera.hpp"

class Entity;

struct Scene
{
	Entity SpawnEntity(const std::string& name);
	void DestroyEntity(Entity entity);

	void Render(Camera& editorCamera);

private:
	entt::registry m_Registry;
	std::vector<entt::entity> m_Entities;

	friend class Entity;
};