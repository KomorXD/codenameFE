#pragma once

#include "entt/entt.hpp"
#include "Scene.hpp"
#include "Components.hpp"

class Entity
{
public:
	Entity() = default;
	Entity(entt::entity handle, Scene* scene);

	inline entt::entity Handle() const { return m_Handle; }

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		assert(!HasComponent<T>() && "Entity already has the component");
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
		assert(HasComponent<T>() && "Entity doesn't have the component");
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
		assert(HasComponent<T>() && "Entity doesn't have the component");
		m_Scene->m_Registry.remove<T>(m_Handle);
	}

	template<>
	void RemoveComponent<TransformComponent>()
	{
		assert(false && "Can't remove the transform component");
	}

	template<>
	void RemoveComponent<TagComponent>()
	{
		assert(false && "Can't remove the tag component");
	}

private:
	entt::entity m_Handle = entt::null;
	Scene* m_Scene = nullptr;
};