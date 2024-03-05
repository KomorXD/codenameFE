#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "../renderer/Renderer.hpp"
#include "../Logger.hpp"

Entity Scene::SpawnEntity(const std::string& name)
{
	Entity ent(m_Registry.create(), this);
	ent.AddComponent<TagComponent>().Tag = name.empty() ? "Entity" : name;
	ent.AddComponent<TransformComponent>();
	m_Entities.push_back(ent.Handle());

	return ent;
}

void Scene::DestroyEntity(Entity entity)
{
	m_Entities.erase(std::find(m_Entities.begin(), m_Entities.end(), entity.Handle()));
	m_Registry.destroy(entity.Handle());
}

void Scene::Render(Camera& editorCamera)
{
	Renderer::SceneBegin(editorCamera);

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
		auto view = m_Registry.view<TransformComponent, MeshComponent, MaterialComponent>(entt::exclude<PointLightComponent>);
		for (entt::entity entity : view)
		{
			auto[transform, mesh, material] = view.get<TransformComponent, MeshComponent, MaterialComponent>(entity);
			Renderer::SubmitMesh(transform.ToMat4(), mesh, material, (int32_t)entity);
		}
	}

	Renderer::SceneEnd();
	Renderer::SetLight(true);
	Renderer::SceneBegin(editorCamera);

	// Render meshes with point light component (no shading)
	{
		auto view = m_Registry.view<TransformComponent, PointLightComponent, MeshComponent, MaterialComponent>();
		for (entt::entity entity : view)
		{
			auto [transform, mesh, material] = view.get<TransformComponent, MeshComponent, MaterialComponent>(entity);
			Renderer::SubmitMesh(transform.ToMat4(), mesh, material, (int32_t)entity);
		}
	}

	Renderer::SceneEnd();
	Renderer::SetLight(false);
}
