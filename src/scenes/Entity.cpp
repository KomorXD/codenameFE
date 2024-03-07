#include "Entity.hpp"

Entity::Entity(entt::entity handle, Scene* scene)
	: m_Handle(handle), m_Scene(scene)
{
}

Entity::Entity(const Entity& other)
	: m_Handle(other.m_Handle), m_Scene(other.m_Scene)
{
}
