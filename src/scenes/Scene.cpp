#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "../renderer/Renderer.hpp"
#include "../Logger.hpp"

Entity Scene::SpawnEntity(const std::string& name)
{
	Entity ent(m_Registry.create((entt::entity)(m_Entities.size() + 1)), this);
	ent.AddComponent<TagComponent>().Tag = name.empty() ? "Entity" : name;
	ent.AddComponent<TransformComponent>();
	m_Entities.push_back(ent);
	return ent;
}

void Scene::DestroyEntity(Entity entity)
{
	m_Entities.erase(std::find_if(m_Entities.begin(), m_Entities.end(), [&](const Entity& other) { return entity.Handle() == other.Handle(); }));
	m_Registry.destroy(entity.Handle());
}

void Scene::Render(Camera& editorCamera)
{
	Renderer::SceneBegin(editorCamera);

	// Add directional lights
	{
		auto view = m_Registry.view<TransformComponent, DirectionalLightComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, light] = view.get<TransformComponent, DirectionalLightComponent>(entity);
			Renderer::AddDirectionalLight(transform, light);
		}
	}

	// Add spotlights
	{
		auto view = m_Registry.view<TransformComponent, SpotLightComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, light] = view.get<TransformComponent, SpotLightComponent>(entity);
			Renderer::AddSpotLight(transform, light);
		}
	}

	// Add point lights
	{
		auto view = m_Registry.view<TransformComponent, PointLightComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
			Renderer::AddPointLight(transform.Position, light);
		}
	}

	// Render meshes without point light component
	{
		auto view = m_Registry.view<TransformComponent, MeshComponent, MaterialComponent>(entt::exclude<DirectionalLightComponent, PointLightComponent, SpotLightComponent>);
		for (entt::entity entity : view)
		{
			auto[transform, mesh, material] = view.get<TransformComponent, MeshComponent, MaterialComponent>(entity);
			Renderer::SubmitMesh(transform.ToMat4(), mesh, material, (int32_t)entity);
		}
	}

	Renderer::SceneEnd();
	Renderer::SetLight(true);
	Renderer::SceneBegin(editorCamera);

	// Render meshes with light component (no shading)
	{
		auto view = m_Registry.view<TransformComponent, DirectionalLightComponent, MeshComponent, MaterialComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, mesh, material, light] = view.get<TransformComponent, MeshComponent, MaterialComponent, DirectionalLightComponent>(entity);
			MaterialComponent intensified = material;
			intensified.Color = glm::vec4(glm::vec3(material.Color * light.Intensity), 1.0f);
			Renderer::SubmitMesh(transform.ToMat4(), mesh, intensified, (int32_t)entity);
		}
	}
	{
		auto view = m_Registry.view<TransformComponent, PointLightComponent, MeshComponent, MaterialComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, mesh, material, light] = view.get<TransformComponent, MeshComponent, MaterialComponent, PointLightComponent>(entity);
			MaterialComponent intensified = material;
			intensified.Color = glm::vec4(glm::vec3(material.Color * light.Intensity), 1.0f);
			Renderer::SubmitMesh(transform.ToMat4(), mesh, intensified, (int32_t)entity);
		}
	}
	{
		auto view = m_Registry.view<TransformComponent, SpotLightComponent, MeshComponent, MaterialComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, mesh, material, light] = view.get<TransformComponent, MeshComponent, MaterialComponent, SpotLightComponent>(entity);
			MaterialComponent intensified = material;
			intensified.Color = glm::vec4(glm::vec3(material.Color * light.Intensity), 1.0f);
			Renderer::SubmitMesh(transform.ToMat4(), mesh, intensified, (int32_t)entity);
		}
	}

	Renderer::SceneEnd();
	Renderer::SetLight(false);
}
