#include "Entity.hpp"

Entity::Entity(entt::entity handle, Scene* scene)
	: m_Handle(handle), m_Scene(scene)
{
}

Entity::Entity(const Entity& other)
	: m_Handle(other.m_Handle), m_Scene(other.m_Scene)
{
}

Entity::Entity(Entity&& other) noexcept
{
	*this = std::move(other);
}

Entity Entity::Clone()
{
	if (m_Handle == entt::null)
	{
		return Entity();
	}

	Entity newEntity = m_Scene->SpawnEntity(GetComponent<TagComponent>().Tag);
	newEntity.GetComponent<TransformComponent>() = GetComponent<TransformComponent>();

	if (HasComponent<MaterialComponent>())
	{
		newEntity.AddComponent<MaterialComponent>() = GetComponent<MaterialComponent>();
	}

	if (HasComponent<MeshComponent>())
	{
		newEntity.AddComponent<MeshComponent>() = GetComponent<MeshComponent>();
	}

	if (HasComponent<DirectionalLightComponent>())
	{
		newEntity.AddComponent<DirectionalLightComponent>() = GetComponent<DirectionalLightComponent>();
	}

	if (HasComponent<PointLightComponent>())
	{
		newEntity.AddComponent<PointLightComponent>() = GetComponent<PointLightComponent>();
	}

	if (HasComponent<SpotLightComponent>())
	{
		newEntity.AddComponent<SpotLightComponent>() = GetComponent<SpotLightComponent>();
	}

	return newEntity;
}