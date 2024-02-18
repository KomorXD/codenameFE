#include "EditorLayer.hpp"

#include "../Application.hpp"
#include "../Timer.hpp"
#include "../renderer/Renderer.hpp"

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

EditorLayer::EditorLayer()
	: m_EditorCamera(90.0f, 16.0f / 9.0f, 0.1f, 1000.0f)
{
	FUNC_PROFILE();

	const WindowSpec& spec = Application::Instance()->Spec();
	Event dummyEv{};
	dummyEv.Type = Event::WindowResized;
	dummyEv.Size.Width = spec.Width;
	dummyEv.Size.Height = spec.Height;

	m_EditorCamera.OnEvent(dummyEv);
}

void EditorLayer::OnAttach()
{
	FUNC_PROFILE();
}

void EditorLayer::OnEvent(Event& ev)
{
	if(ev.Type == Event::KeyPressed)
	{
		if (ev.Key.Code == Key::Escape)
		{
			Application::Instance()->Shutdown();
		}

		m_EditorCamera.OnEvent(ev);

		return;
	}

	if(ev.Type == Event::WindowResized)
	{
		Viewport viewport{ 0, 0, ev.Size.Width, ev.Size.Height };
		Renderer::OnWindowResize(viewport);
		m_EditorCamera.OnEvent(ev);

		return;
	}
	
	m_EditorCamera.OnEvent(ev);
}

void EditorLayer::OnUpdate(float ts)
{
	m_EditorCamera.OnUpdate(ts);
}

void EditorLayer::OnTick()
{
}

void EditorLayer::OnRender()
{
	Renderer::Clear();
	Renderer::ClearColor({ 0.3f, 0.4f, 0.5f, 1.0f });

	Renderer::SceneBegin(m_EditorCamera);
	Renderer::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Renderer::SceneEnd();

	ImGui::Begin("Hiii!!!!!!");
	if (ImGui::CollapsingHeader("Camera"))
	{
		ImGui::DragFloat3("Position", glm::value_ptr(m_EditorCamera.m_Position), 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Pitch", &m_EditorCamera.m_Pitch, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Yaw", &m_EditorCamera.m_Yaw, 1.0f, -FLT_MAX, FLT_MAX);
	}
	ImGui::End();
}
