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

	m_ScreenFB = std::make_unique<Framebuffer>();
	m_ScreenFB->AttachTexture((uint32_t)spec.Width, (uint32_t)spec.Height);
	m_ScreenFB->AttachRenderBuffer((uint32_t)spec.Width, (uint32_t)spec.Height);
	m_ScreenFB->UnbindBuffer();

	m_TargetFB = std::make_unique<Framebuffer>();
	m_TargetFB->AttachTexture((uint32_t)spec.Width, (uint32_t)spec.Height);
	m_TargetFB->AttachRenderBuffer((uint32_t)spec.Width, (uint32_t)spec.Height);
	m_TargetFB->UnbindBuffer();

	m_MainMFB = std::make_unique<MultisampledFramebuffer>(16);
	m_MainMFB->AttachTexture((uint32_t)spec.Width, (uint32_t)spec.Height);
	m_MainMFB->AttachRenderBuffer((uint32_t)spec.Width, (uint32_t)spec.Height);
	m_MainMFB->UnbindBuffer();

	m_DepthFB = std::make_unique<Framebuffer>();
	m_DepthFB->AttachDepthBuffer(1024, 1024);
	m_DepthFB->UnbindBuffer();
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
		uint32_t width = ev.Size.Width;
		uint32_t height = ev.Size.Height;

		Renderer::OnWindowResize({ 0, 0, ev.Size.Width, ev.Size.Height });
		m_EditorCamera.OnEvent(ev);

		m_ScreenFB->BindBuffer();
		m_ScreenFB->ResizeTexture(width, height);
		m_ScreenFB->ResizeRenderBuffer(width, height);
		m_ScreenFB->UnbindBuffer();

		m_TargetFB->BindBuffer();
		m_TargetFB->ResizeTexture(width, height);
		m_TargetFB->ResizeRenderBuffer(width, height);
		m_TargetFB->UnbindBuffer();

		m_MainMFB->BindBuffer();
		m_MainMFB->ResizeTexture(width, height);
		m_MainMFB->ResizeRenderBuffer(width, height);
		m_MainMFB->UnbindBuffer();

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
static glm::vec3 s_Dir = glm::vec3(-0.5f, -1.0f, -0.2f);
static float s_Linear    = 0.022f;
static float s_Quadratic = 0.0019f;
static float s_CutOff = 12.5f;
static float s_OuterCutOff = 17.5f;

void EditorLayer::OnRender()
{
	static bool s_Blur = false;

	// Depth pass - shadow map
	glm::ivec2 bufferSize = m_DepthFB->ShadowMapSize();
	Renderer::OnWindowResize({ 0, 0, bufferSize.x, bufferSize.y });
	m_DepthFB->BindBuffer();
	Renderer::Clear(GL_DEPTH_BUFFER_BIT);
	Renderer::RenderDepth();
	Renderer::SceneBegin(m_EditorCamera);
	RenderScene();
	Renderer::SceneEnd();
	m_DepthFB->UnbindBuffer();
	
	// Normal pass
	bufferSize = m_MainMFB->RenderDimensions();
	Renderer::OnWindowResize({ 0, 0, bufferSize.x, bufferSize.y });
	m_MainMFB->BindBuffer();
	m_MainMFB->BindRenderBuffer();
	Renderer::Clear();
	Renderer::ClearColor({ 0.3f, 0.4f, 0.5f, 1.0f });
	Renderer::RenderDefault();
	Renderer::SceneBegin(m_EditorCamera);
	Renderer::AddShadowMap(m_DepthFB);
	Renderer::SetBlur(s_Blur);
	RenderScene();
	Renderer::SceneEnd();

	// MSAA stuff
	glm::uvec2 dim = m_MainMFB->RenderDimensions();
	m_MainMFB->BlitBuffers(dim.x, dim.y, m_ScreenFB->GetFramebufferID());
	m_MainMFB->UnbindRenderBuffer();
	m_MainMFB->UnbindBuffer();
	m_TargetFB->BindBuffer();
	m_TargetFB->BindRenderBuffer();
	m_ScreenFB->BindTexture();
	Renderer::DrawScreenQuad();
	m_TargetFB->UnbindRenderBuffer();
	m_TargetFB->UnbindBuffer();

	// GUI
	ImGui::SetNextWindowPos({ 0.0f, 0.0f });
	ImGui::SetNextWindowSize({ (float)Application::Instance()->Spec().Width, (float)Application::Instance()->Spec().Height });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::Image((ImTextureID)m_TargetFB->GetTextureID(), ImGui::GetContentRegionAvail(), { 0.0f, 1.0f }, { 1.0f, 0.0f });
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::Begin("Depth map");
	ImGui::Image((ImTextureID)m_DepthFB->GetTextureID(), { 512.0f, 512.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
	ImGui::End();

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

void EditorLayer::RenderScene()
{
	Renderer::AddDirectionalLight({
		   .Direction = s_Dir,
		   .Color = s_LightCol
		});
	/*Renderer::AddPointLight({
		.Position = s_LightPos,
		.Color = s_LightCol,
		.LinearTerm = s_Linear,
		.QuadraticTerm = s_Quadratic
	});*/

	// Renderer::DrawCube(s_LightPos, glm::vec3(1.0f), glm::vec4(s_LightCol, 1.0f));
	Renderer::DrawCube(glm::vec3(0.0f), { 50.0f, 0.2f, 50.0f }, *s_GrassTexture);

	Renderer::DrawQuad({ 0.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, *s_PlankTexture);
	Renderer::DrawQuad({ 3.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Renderer::DrawQuad({ -3.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, *s_PlankTexture);

	Renderer::DrawLine({ 10.0f, 0.0f,  10.0f }, { -10.0f, 0.0f,  10.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Renderer::DrawLine({ -10.0f, 0.0f,  10.0f }, { -10.0f, 0.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
	Renderer::DrawLine({ -10.0f, 0.0f, -10.0f }, { 10.0f, 0.0f, -10.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });
	Renderer::DrawLine({ 10.0f, 0.0f, -10.0f }, { 10.0f, 0.0f,  10.0f }, { 0.0f, 1.0f, 1.0f, 1.0f });

	constexpr float radius = 10.0f;
	constexpr uint32_t count = 10;
	constexpr float step = 2.0f * glm::pi<float>() / count;
	bool textured = false;
	for (uint32_t i = 0; i < count; i++)
	{
		glm::vec3 pos = { glm::cos(i * step) * radius, 2.0f, glm::sin(i * step) * radius };

		if (textured)
		{
			Renderer::DrawCube(pos, glm::vec3(2.0f), *s_PlankTexture);
		}
		else
		{
			Renderer::DrawCube(pos, glm::vec3(1.0f), *s_SandTexture);
		}

		textured = !textured;
	}
}
