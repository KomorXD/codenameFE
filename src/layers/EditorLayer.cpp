#include "EditorLayer.hpp"

#include <glad/glad.h>
#include "../Application.hpp"

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
		if (ev.Key.Code == Key::Escape)
		{
			Application::Instance()->Shutdown();
		}

		break;
		
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.66f, 0.11f, 0.22f, 1.0f);
}
