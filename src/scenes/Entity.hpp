#pragma once

#include "entt/entt.hpp"
#include "Scene.hpp"

class Entity
{
public:
	Entity() = default;
	Entity(entt::entity handle, Scene* scene);

	inline entt::entity Handle() const { return m_Handle; }

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	T& AddOrReplaceComponent(Args&&... args)
	{
		return m_Scene->m_Registry.emplace_or_replace<T>(m_Handle, std::forward<Args>(args)...);
	}

	template<typename T>
	T& GetComponent()
	{
		return m_Scene->m_Registry.get<T>(m_Handle);
	}

	template<typename T>
	bool HasComponent()
	{
		return m_Scene->m_Registry.all_of<T>(m_Handle);
	}

	template<typename T>
	void RemoveComponent()
	{
		m_Scene->m_Registry.remove<T>(m_Handle);
	}

private:
	entt::entity m_Handle = entt::null;
	Scene* m_Scene = nullptr;
};