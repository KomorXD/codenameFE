#include "EditorLayer.hpp"

#include "../Application.hpp"
#include "../Timer.hpp"
#include "../renderer/Renderer.hpp"

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

static std::unique_ptr<Texture> s_PlankTexture;
static std::unique_ptr<Texture> s_GrassTexture;
static std::unique_ptr<Texture> s_SandTexture;

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
	m_EditorCamera.Position = { 0.0f, 10.0f, 20.0f };

	Renderer::OnWindowResize({ 0, 0, spec.Width, spec.Height });

	s_PlankTexture = std::make_unique<Texture>("resources/textures/cbbl.png");
	s_GrassTexture = std::make_unique<Texture>("resources/textures/grass.jpg");
	s_SandTexture = std::make_unique<Texture>("resources/textures/sand.png");
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
		Renderer::OnWindowResize({ 0, 0, ev.Size.Width, ev.Size.Height });
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

static glm::vec3 s_LightPos = glm::vec3(0.0f, 5.0f, 0.0f);
static glm::vec3 s_LightCol = glm::vec3(1.0f);
static glm::vec3 s_Dir = glm::vec3(0.0f, -1.0f, 0.0f);
static float s_Linear    = 0.022f;
static float s_Quadratic = 0.0019f;
static float s_CutOff = 12.5f;
static float s_OuterCutOff = 17.5f;

void EditorLayer::OnRender()
{
	static bool s_Blur = false;
	
	Renderer::Clear();
	Renderer::ClearColor({ 0.3f, 0.4f, 0.5f, 1.0f });
	
	Renderer::SetBlur(s_Blur);
	Renderer::RenderStart();
	Renderer::Clear();
	Renderer::ClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });

	Renderer::SceneBegin(m_EditorCamera);
	Renderer::AddPointLight({
		.Position = s_LightPos,
		.Color = s_LightCol,
		.LinearTerm = s_Linear,
		.QuadraticTerm = s_Quadratic
	});
	
	Renderer::DrawCube(s_LightPos, glm::vec3(1.0f), glm::vec4(s_LightCol, 1.0f));
	Renderer::DrawCube(glm::vec3(0.0f), { 15.0f, 0.2f, 15.0f }, *s_GrassTexture);

	Renderer::DrawQuad({ 0.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, *s_PlankTexture);
	Renderer::DrawQuad({ 3.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Renderer::DrawQuad({ -3.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, *s_PlankTexture);

	Renderer::DrawLine({  10.0f, 0.0f,  10.0f }, { -10.0f, 0.0f,  10.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Renderer::DrawLine({ -10.0f, 0.0f,  10.0f }, { -10.0f, 0.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
	Renderer::DrawLine({ -10.0f, 0.0f, -10.0f }, {  10.0f, 0.0f, -10.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });
	Renderer::DrawLine({  10.0f, 0.0f, -10.0f }, {  10.0f, 0.0f,  10.0f }, { 0.0f, 1.0f, 1.0f, 1.0f });
	
	constexpr float radius = 10.0f;
	constexpr uint32_t count = 20;
	constexpr float step = 2.0f * glm::pi<float>() / count;
	bool textured = false;
	for (uint32_t i = 0; i < count; i++)
	{
		glm::vec3 pos = { glm::cos(i * step) * radius, 5.0f, glm::sin(i * step) * radius };

		if (textured)
		{
			Renderer::DrawCube(pos, { 1.0f, 1.0f, 1.0f }, *s_PlankTexture);
		}
		else
		{
			Renderer::DrawCube(pos, { 1.0f, 1.0f, 1.0f }, *s_SandTexture);
		}

		textured = !textured;
	}

	Renderer::SceneEnd();
	Renderer::RenderEnd();

	ImGui::SetNextWindowPos({ 0.0f, 0.0f });
	ImGui::SetNextWindowSize({ (float)Application::Instance()->Spec().Width, (float)Application::Instance()->Spec().Height });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::Image((ImTextureID)Renderer::RenderTextureID(), ImGui::GetContentRegionAvail(), { 0.0f, 1.0f }, { 1.0f, 0.0f });
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::Begin("Hiii!!!!!!");
	ImGui::Checkbox("Blur", &s_Blur);

	if (ImGui::CollapsingHeader("Camera"))
	{
		ImGui::DragFloat3("Cam position", glm::value_ptr(m_EditorCamera.Position), 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Pitch", &m_EditorCamera.m_Pitch, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Yaw", &m_EditorCamera.m_Yaw, 1.0f, -FLT_MAX, FLT_MAX);
	}

	if (ImGui::CollapsingHeader("Point light source"))
	{
		ImGui::DragFloat3("Light position", glm::value_ptr(s_LightPos), 0.05f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat3("Color", glm::value_ptr(s_LightCol), 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("Direction", glm::value_ptr(s_Dir), 0.01f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Linear term", &s_Linear, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat("Quadratic term", &s_Quadratic, 0.001f, 0.0f, 2.0f);
		ImGui::DragFloat("Cut off", &s_CutOff, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Outer cut off", &s_OuterCutOff, 0.1f, 0.0f, 100.0f);
	}
	ImGui::End();
}
