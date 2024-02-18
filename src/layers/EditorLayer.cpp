#include "EditorLayer.hpp"

#include <glad/glad.h>
#include "../Application.hpp"
#include "../renderer/Renderer.hpp"

#include <imgui/imgui.h>

EditorLayer::EditorLayer()
{
}

void EditorLayer::OnAttach()
{
}

void EditorLayer::OnEvent(Event& ev)
{
	switch (ev.Type)
	{
	case Event::KeyPressed:
	{
		if (ev.Key.Code == Key::Escape)
		{
			Application::Instance()->Shutdown();
		}

		break;
	}

	case Event::WindowResized:
	{
		Viewport viewport{ 0, 0, ev.Size.Width, ev.Size.Height };
		Renderer::OnWindowResize(viewport);

		break;
	}
		
	default:
		break;
	}
}

void EditorLayer::OnUpdate(float ts)
{
}

void EditorLayer::OnTick()
{
}

void EditorLayer::OnRender()
{
	Renderer::Clear();
	Renderer::ClearColor({ 0.3f, 0.4f, 0.5f, 1.0f });

	Renderer::SceneBegin();
	Renderer::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Renderer::SceneEnd();

	ImGui::Begin("Hiii!!!!!!");
	if (ImGui::Button("bye:("))
	{
		Application::Instance()->Shutdown();
	}
	ImGui::End();
}
