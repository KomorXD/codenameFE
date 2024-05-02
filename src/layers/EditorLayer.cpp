#include "EditorLayer.hpp"
#include "MaterialEditLayer.hpp"

#include "../Application.hpp"
#include "../Timer.hpp"
#include "../Logger.hpp"
#include "../scenes/Entity.hpp"
#include "../renderer/AssetManager.hpp"
#include "../RandomUtils.hpp"

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

EditorLayer::EditorLayer()
	: m_EditorCamera(90.0f, 16.0f / 9.0f, 0.1f, 1000.0f)
{
	FUNC_PROFILE();

	m_GizmoMode = ImGuizmo::WORLD;

	std::shared_ptr<Texture> hdrEnvMap = std::make_shared<Texture>("resources/textures/env_maps/default.hdr", TextureFormat::RGB16F);
	hdrEnvMap->SetWrap(GL_CLAMP_TO_EDGE);
	m_SkyboxFB = Renderer::CreateEnvCubemap(hdrEnvMap, { 1024, 1024 });

	const WindowSpec& spec = Application::Instance()->Spec();
	Event dummyEv{};
	dummyEv.Type = Event::WindowResized;
	dummyEv.Size.Width = spec.Width * 0.6f;
	dummyEv.Size.Height = spec.Height;

	m_EditorCamera.OnEvent(dummyEv);
	m_EditorCamera.Position = { 0.0f, 5.0f, -10.0f };

	Renderer::OnWindowResize({ 0, 0, (int32_t)(spec.Width * 0.6f), spec.Height });

	glm::uvec2 fbSize((uint32_t)(spec.Width * 0.6f), spec.Height);
	m_MainFB = std::make_unique<Framebuffer>(16);
	m_MainFB->AddRenderbuffer({
		.Type = RenderbufferType::DEPTH_STENCIL,
		.Size = fbSize
	});
	m_MainFB->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_2D_MULTISAMPLE,
		.Format = TextureFormat::RGBA16F,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = fbSize,
		.GenMipmaps = false
	});
	m_MainFB->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_2D_MULTISAMPLE,
		.Format = TextureFormat::RGBA8,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = fbSize,
		.GenMipmaps = false
	});
	m_MainFB->Unbind();

	m_ScreenFB = std::make_unique<Framebuffer>(1);
	m_ScreenFB->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_2D,
		.Format = TextureFormat::RGBA16F,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = fbSize,
		.GenMipmaps = false
	});
	m_ScreenFB->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_2D,
		.Format = TextureFormat::RGBA8,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = fbSize,
		.GenMipmaps = false
	});
	m_ScreenFB->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_2D,
		.Format = TextureFormat::RGBA16F,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = fbSize,
		.GenMipmaps = false
	});
	m_ScreenFB->Unbind();
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
			m_SelectedEntity = Entity();
			return;
		}

		if (ev.Key.Code == Key::Delete && m_SelectedEntity.Handle() != entt::null && !m_IsGizmoUsed)
		{
			m_Scene.DestroyEntity(m_SelectedEntity);
			m_SelectedEntity = Entity();
			return;
		}

		if (ev.Key.Code == Key::D && ev.Key.CtrlPressed && m_SelectedEntity.Handle() != entt::null)
		{
			m_SelectedEntity = m_SelectedEntity.Clone();
			return;
		}
		
		if (!Input::IsKeyPressed(Key::LeftAlt)) // FPS mode when left alt is pressed
		{
			switch (ev.Key.Code)
			{
			case Key::Q:	m_GizmoOp = -1;				     return;
			case Key::W:	m_GizmoOp = ImGuizmo::TRANSLATE; return;
			case Key::E:	m_GizmoOp = ImGuizmo::ROTATE;    return;
			case Key::R:	m_GizmoOp = ImGuizmo::SCALE;	 return;
			}
		}

		m_EditorCamera.OnEvent(ev);
		return;
	}

	if (!m_LockFocus && ev.Type == Event::MouseButtonPressed && ev.MouseButton.Button == MouseButton::Left && m_ViewportHovered)
	{
		glm::vec2 mousePos = Input::GetMousePosition() - glm::vec2(((float)Application::Instance()->Spec().Width / 5.0f), 0.0f);
		glm::u8vec4 pixel = m_ScreenFB->GetPixelAt(mousePos, 1);

		if (pixel == glm::u8vec4(0))
		{
			m_SelectedEntity = Entity();
			return;
		}

		uint32_t redContrib   = pixel.r * 65025;
		uint32_t greenContrib = pixel.g * 255;
		uint32_t blueContrib  = pixel.b;
		uint32_t pixelID = redContrib + greenContrib + blueContrib;
		m_SelectedEntity = Entity((entt::entity)(uint32_t)(pixelID - 1), &m_Scene);

		return;
	}

	if(ev.Type == Event::WindowResized)
	{
		uint32_t width = (uint32_t)(ev.Size.Width * 0.6f);
		uint32_t height = ev.Size.Height;

		Renderer::OnWindowResize({ 0, 0, (int32_t)(ev.Size.Width * 0.6f), ev.Size.Height });
		m_EditorCamera.OnEvent(ev);

		m_ScreenFB->Bind();
		m_ScreenFB->ResizeEverything({ width, height });
		m_ScreenFB->Unbind();

		m_MainFB->Bind();
		m_MainFB->ResizeEverything({ width, height });
		m_MainFB->Unbind();

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
	RenderScenePanel();
	RenderViewport();

	ImGui::SetNextWindowPos({ ((float)Application::Instance()->Spec().Width * 0.2f), 0.0f});
	ImGui::SetNextWindowSize({ (float)Application::Instance()->Spec().Width * 0.6f, (float)Application::Instance()->Spec().Height });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::Image((ImTextureID)m_ScreenFB->GetColorAttachmentID(2), ImGui::GetContentRegionAvail(), { 0.0f, 1.0f }, { 1.0f, 0.0f });
	m_ViewportHovered = ImGui::IsWindowHovered();
	m_ViewportFocused = ImGui::IsWindowFocused();
	RenderGizmo();
	ImGui::PopStyleVar();
	ImGui::End();

	RenderEntityData();
}

void EditorLayer::RenderScenePanel()
{
	glm::vec2 windowSize = { Application::Instance()->Spec().Width, Application::Instance()->Spec().Height };
	ImGui::SetNextWindowPos({ 0.0f, 0.0f });
	ImGui::SetNextWindowSize({ (float)Application::Instance()->Spec().Width * 0.2f, (float)Application::Instance()->Spec().Height });
	ImGui::Begin("Scene panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	ImVec2 avSpace = ImGui::GetContentRegionAvail();
	ImGui::Text("Spawned entities");
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	ImGui::BeginChild("Spawned entites", ImVec2(avSpace.x, avSpace.y / 5.0f), true);
	for (Entity& entity : m_Scene.m_Entities)
	{
		ImGui::PushID((uint32_t)entity.Handle());
		if (ImGui::Selectable(entity.GetComponent<TagComponent>().Tag.c_str(), entity.Handle() == m_SelectedEntity.Handle()))
		{
			m_SelectedEntity = entity;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	float buttonWidth = ImGui::GetContentRegionAvail().x / 3.0f;
	ImVec2 buttonSize(buttonWidth, 0.0f);
	if (ImGui::PrettyButton("New entity", buttonSize))
	{
		ImGui::OpenPopup("new_entity_group");
	}

	if (ImGui::PrettyButton("Material editor", buttonSize))
	{
		Application::Instance()->PushLayer(std::make_unique<MaterialEditLayer>(m_Scene.m_Entities, m_SkyboxFB));
	}

	if (ImGui::PrettyButton("Reload shaders", buttonSize))
	{
		Renderer::ReloadShaders();
	}

	if (ImGui::BeginPopup("new_entity_group"))
	{
		if (ImGui::MenuItem("Empty entity"))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Empty entity");
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem("Plane"))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Plane");
			m_SelectedEntity.GetComponent<TransformComponent>().Rotation = { glm::half_pi<float>(), 0.0f, 0.0f};
			m_SelectedEntity.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_PLANE;
			m_SelectedEntity.AddComponent<MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem("Cube"))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Cube");
			m_SelectedEntity.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_CUBE;
			m_SelectedEntity.AddComponent<MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem("Sphere"))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Sphere");
			m_SelectedEntity.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_SPHERE;
			m_SelectedEntity.AddComponent<MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::BeginPrettyCombo("Gizmo mode", m_GizmoMode == ImGuizmo::WORLD ? "World" : "Local",
		[this]()
		{
			if (ImGui::Selectable("World", m_GizmoMode == ImGuizmo::WORLD))
			{
				m_GizmoMode = ImGuizmo::WORLD;
			}

			if (ImGui::Selectable("Local", m_GizmoMode == ImGuizmo::LOCAL))
			{
				m_GizmoMode = ImGuizmo::LOCAL;
			}
		}
	);

	if (ImGui::PrettyButton("Add environment map"))
	{
		std::optional<std::string> fileOpt = OpenFileDialog(std::filesystem::current_path().string());
		if (fileOpt.has_value())
		{
			std::shared_ptr<Texture> hdrFile = std::make_shared<Texture>(fileOpt.value(), TextureFormat::RGB16F);
			hdrFile->SetWrap(GL_CLAMP_TO_EDGE);
			m_SkyboxFB = Renderer::CreateEnvCubemap(hdrFile, { 1024, 1024 });
		}
	}

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(16.0f);
		ImGui::PrettyDragFloat3("Position", glm::value_ptr(m_EditorCamera.Position), 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::PrettyDragFloat("Exposure", &m_EditorCamera.Exposure, 0.001f, 0.0f, FLT_MAX);
		ImGui::PrettyDragFloat("Brightness", &m_EditorCamera.Gamma, 0.001f, 0.0f, FLT_MAX);
		ImGui::PrettyDragFloat("Bloom strength", &m_BloomStrength, 0.001f, 0.0f, FLT_MAX);
		ImGui::PrettyDragFloat("Bloom threshold", &m_BloomThreshold, 0.001f, 0.0f, FLT_MAX);
		ImGui::PrettyDragFloat("Pitch", &m_EditorCamera.m_Pitch, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::PrettyDragFloat("Yaw", &m_EditorCamera.m_Yaw, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::Checkbox("Bloom", &m_UseBloom);
		ImGui::Checkbox("Wireframe", &m_DrawWireframe);
		ImGui::Checkbox("Grid", &m_DrawGrid);
		ImGui::Unindent(16.0f);
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("Stats", 2))
	{
		ImGui::Indent(16.0f);

		ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		const AppStats& stats = Application::Instance()->Stats();

		ImGui::TableNextColumn();
		ImGui::Text("Frame time");
		ImGui::TableNextColumn();
		ImGui::Text("%.3fms", stats.FrameTime);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("FPS");
		ImGui::TableNextColumn();
		ImGui::Text("%u", static_cast<uint32_t>(1000.0f / stats.FrameTime));

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Update time");
		ImGui::TableNextColumn();
		ImGui::Text("%.3fms", stats.UpdateTime);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Render time");
		ImGui::TableNextColumn();
		ImGui::Text("%.3fms", stats.RenderTime);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("ImGui render time");
		ImGui::TableNextColumn();
		ImGui::Text("%.3fms", stats.ImGuiRenderTime);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Point CUBE shadow pass");
		ImGui::TableNextColumn();
		ImGui::Text("%.3fms", m_Stats.PointLightShadowPassTime);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Spotlight shadow pass");
		ImGui::TableNextColumn();
		ImGui::Text("%.3fms", m_Stats.SpotlightShadowPassTime);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Draw calls");
		ImGui::TableNextColumn();
		ImGui::Text("%u", m_Stats.RenderPassDrawCalls);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Absolute draw calls");
		ImGui::TableNextColumn();
		ImGui::Text("%u", m_Stats.DrawCalls);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Rendered meshes");
		ImGui::TableNextColumn();
		ImGui::Text("%u", m_Stats.ObjectsRendered);

		ImGui::EndTable();
		ImGui::Unindent(16.0f);
	}

	ImGui::End();
}

void EditorLayer::RenderViewport()
{
	Renderer::ResetStats();

	m_Scene.RenderShadowMaps();
	m_MainFB->Bind();
	m_MainFB->BindRenderbuffer();
	m_MainFB->DrawToColorAttachment(0, 0);
	m_MainFB->DrawToColorAttachment(1, 1);
	m_MainFB->FillDrawBuffers();
	Renderer::ClearColor(m_BgColor);
	Renderer::Clear();

	// Render selected entity to stencil buffer
	if (m_SelectedEntity.Handle() != entt::null && m_SelectedEntity.HasComponent<MeshComponent>())
	{
		TransformComponent& transform = m_SelectedEntity.GetComponent<TransformComponent>();
		MeshComponent& mesh	= m_SelectedEntity.GetComponent<MeshComponent>();
		
		Renderer::SetStencilFunc(GL_ALWAYS, 1, 0xFF);
		Renderer::SetStencilMask(0xFF);
		Renderer::DisableDepthTest();
		Renderer::SetLight(true);
		Renderer::SceneBegin(m_EditorCamera);
		Renderer::SubmitMesh(transform.ToMat4(), mesh, {}, (int32_t)m_SelectedEntity.Handle());
		Renderer::SceneEnd();
		Renderer::EnableDepthTest();
		Renderer::SetLight(false);
		Renderer::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Normal pass
	Renderer::SetWireframe(m_DrawWireframe);
	Renderer::SetStencilFunc(GL_ALWAYS, 0, 0xFF);
	Renderer::SetStencilMask(0x00);
	Renderer::DrawSkybox(m_SkyboxFB);
	m_MainFB->ClearColorAttachment(1);
	m_Scene.Render(m_EditorCamera);
	Renderer::SetWireframe(false);

	// Grid
	if (m_DrawGrid)
	{
		constexpr float distance = 100.0f;
		constexpr float offset = 2.0f;
		constexpr glm::vec4 lineColor(glm::vec3(0.22f), 1.0f);
		constexpr glm::vec4 darkLineColor(glm::vec3(0.11f), 1.0f);
		constexpr glm::vec4 mainAxisLineColor(0.98f, 0.24f, 0.0f, 1.0f);
		glm::vec3 cameraPos = m_EditorCamera.Position;
		int32_t xOffset = cameraPos.x / (int32_t)offset;
		int32_t zOffset = cameraPos.z / (int32_t)offset;

		Renderer::SceneBegin(m_EditorCamera);
		GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));
		for (float x = -distance + xOffset * offset; x <= distance + xOffset * offset; x += offset)
		{
			glm::vec4 color = (x == 0.0f ? mainAxisLineColor : ((int32_t)x % 10 == 0 ? darkLineColor : lineColor));

			Renderer::DrawLine({ x, 0.0f, -distance + zOffset * offset },
				{ x, 0.0f, distance + zOffset * offset }, color);
		}

		for (float z = -distance + zOffset * offset; z <= distance + zOffset * offset; z += offset)
		{
			glm::vec4 color = (z == 0.0f ? mainAxisLineColor : ((int32_t)z % 10 == 0 ? darkLineColor : lineColor));

			Renderer::DrawLine({ -distance + xOffset * offset, 0.0f, z },
				{ distance + xOffset * offset, 0.0f, z }, color);
		}
		Renderer::SceneEnd();
	}

	// Render selected entity outline
	if (m_SelectedEntity.Handle() != entt::null && m_SelectedEntity.HasComponent<MeshComponent>())
	{
		TransformComponent transform = m_SelectedEntity.GetComponent<TransformComponent>();
		MeshComponent mesh = m_SelectedEntity.GetComponent<MeshComponent>();
		Material material{};

		transform.Scale += 0.1f;
		material.Color = { 0.98f, 0.24f, 0.0f, 1.0f };
		material.AlbedoTextureID = AssetManager::TEXTURE_WHITE;

		Renderer::SetStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		Renderer::SetStencilMask(0x00);
		Renderer::DisableDepthTest();
		Renderer::DisableFaceCulling();
		Renderer::SetLight(true);
		Renderer::SceneBegin(m_EditorCamera);
		Renderer::SubmitMesh(transform.ToMat4(), mesh, material, (int32_t)m_SelectedEntity.Handle());
		Renderer::SceneEnd();
		Renderer::EnableDepthTest();
		Renderer::EnableFaceCulling();
		Renderer::SetLight(false);
	}

	Renderer::SetStencilFunc(GL_ALWAYS, 0, 0xFF);
	Renderer::SetStencilMask(0xFF);
	
	m_MainFB->DrawToColorAttachment(0, 0);
	m_MainFB->DrawToColorAttachment(1, 1);
	m_ScreenFB->DrawToColorAttachment(0, 0);
	m_ScreenFB->DrawToColorAttachment(1, 1);
	m_MainFB->BlitBuffers(0, 0, *m_ScreenFB);
	m_MainFB->BlitBuffers(1, 1, *m_ScreenFB);

	if (m_UseBloom)
	{
		Renderer::SetBloomStrength(m_BloomStrength);
		Renderer::SetBloomThreshold(m_BloomThreshold);
		Renderer::Bloom(m_ScreenFB);
	}
	else
	{
		Renderer::SetBloomStrength(0.0f);
	}
	m_MainFB->BindRenderbuffer();
	m_ScreenFB->Bind();
	m_ScreenFB->BindColorAttachment(0);
	m_ScreenFB->DrawToColorAttachment(2, 2);
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT2));
	Renderer::DrawScreenQuad();
	m_ScreenFB->Unbind();

	m_Stats = Renderer::Stats();
}

void EditorLayer::RenderEntityData()
{
	const WindowSpec& spec = Application::Instance()->Spec();
	ImGui::SetNextWindowPos({ spec.Width * 0.8f, 0.0f });
	ImGui::SetNextWindowSize({ spec.Width * 0.2f, spec.Height * 1.0f });
	ImGui::Begin("Entity panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	if (m_SelectedEntity.Handle() == entt::null)
	{
		ImGui::End();
		return;
	}

	TagComponent& tag = m_SelectedEntity.GetComponent<TagComponent>();
	char buf[64];
	strcpy_s(buf, 64, tag.Tag.data());
	ImGui::Indent(16.0f);
	ImGui::PrettyInputText("Tag", buf, 64);
	ImGui::Unindent(16.0f);
	tag.Tag = buf;

	TransformComponent& transform = m_SelectedEntity.GetComponent<TransformComponent>();
	ImGui::PushID(1);
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(16.0f);
		ImGui::PrettyDragFloat3("Position", glm::value_ptr(transform.Position), 0.05f);
		ImGui::PrettyDragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.05f);
		ImGui::PrettyDragFloat3("Scale",    glm::value_ptr(transform.Scale),    0.05f);
		ImGui::Unindent(16.0f);
	}
	ImGui::PopID();

	ImGui::PushID(2);
	if (m_SelectedEntity.HasComponent<MeshComponent>())
	{
		MeshComponent& mesh = m_SelectedEntity.GetComponent<MeshComponent>();

		if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			ImGui::BeginPrettyCombo("Mesh", AssetManager::GetMesh(mesh.MeshID).Name.c_str(),
				[this, &mesh]()
				{
					const std::unordered_map<int32_t, Mesh>& meshes = AssetManager::AllMeshes();

					for (const auto& [id, meshData] : meshes)
					{
						if (ImGui::Selectable(meshData.Name.c_str(), id == mesh.MeshID))
						{
							mesh.MeshID = id;
						}
					}
				}
			);

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<MeshComponent>();
			}

			ImGui::Unindent(16.0f);
		}
	}
	ImGui::PopID();

	ImGui::PushID(3);
	if (m_SelectedEntity.HasComponent<MaterialComponent>())
	{
		Material& material = AssetManager::GetMaterial(m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID);

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2.0f);
			ImGui::BeginPrettyCombo("Material", material.Name.c_str(),
				[this]()
				{
					for (const auto& [id, mat] : AssetManager::AllMaterials())
					{
						if (ImGui::Selectable(mat.Name.c_str(), id == m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID))
						{
							m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID = id;
						}
					}
				}
			);

			float buttonWidth = ImGui::GetContentRegionAvail().x / 3.0f;
			ImVec2 buttonSize(buttonWidth, 0.0f);
			ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - buttonWidth + 16.0f);

			if (ImGui::PrettyButton("Edit", buttonSize))
			{
				Application::Instance()->PushLayer(
					std::make_unique<MaterialEditLayer>(
						m_Scene.m_Entities,
						m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID,
						m_SkyboxFB
					)
				);
			}

			ImGui::SameLine();

			if (ImGui::PrettyButton("New", buttonSize))
			{
				m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID = AssetManager::AddMaterial(AssetManager::GetMaterial(AssetManager::MATERIAL_DEFAULT));
				Application::Instance()->PushLayer(
					std::make_unique<MaterialEditLayer>(m_Scene.m_Entities, 
						m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID,
						m_SkyboxFB
					)
				);
			}

			ImGui::Separator();
			ImGui::PrettyDragFloat2("Tiling factor", glm::value_ptr(material.TilingFactor), 0.01f, 0.0f, FLT_MAX);
			ImGui::PrettyDragFloat2("Texture offset", glm::value_ptr(material.TextureOffset), 0.01f, -FLT_MAX, FLT_MAX);

			std::shared_ptr<Texture> textures[] = {
				AssetManager::GetTexture(material.AlbedoTextureID),
				AssetManager::GetTexture(material.NormalTextureID),
				AssetManager::GetTexture(material.HeightTextureID),
				AssetManager::GetTexture(material.RoughnessTextureID),
				AssetManager::GetTexture(material.MetallicTextureID),
				AssetManager::GetTexture(material.AmbientOccTextureID)
			};

			ImGui::BeginPrettyCombo("Filtering", textures[0]->Filter() == GL_NEAREST ? "Nearest" : "Linear",
				[this, &textures]()
				{
					if (ImGui::Selectable("Nearest", textures[0]->Filter() == GL_NEAREST))
					{
						for (auto& tex : textures)
						{
							tex->SetFilter(GL_NEAREST);
						}
					}

					if (ImGui::Selectable("Linear", textures[0]->Filter() == GL_LINEAR))
					{
						for (auto& tex : textures)
						{
							tex->SetFilter(GL_LINEAR);
						}
					}
				}
			);

			const std::unordered_map<int32_t, std::string> wrapToString = {
				{ GL_CLAMP_TO_EDGE,		   "Clamp to edge"		  },
				{ GL_CLAMP_TO_BORDER,	   "Clamp to border"	  },
				{ GL_REPEAT,			   "Repeat"				  },
				{ GL_MIRROR_CLAMP_TO_EDGE, "Mirror clamp to edge" },
				{ GL_MIRRORED_REPEAT,	   "Mirrorer repeat"	  }
			};
			std::string preview = wrapToString.at(textures[0]->Wrap());

			ImGui::BeginPrettyCombo("Wrapping", preview.c_str(),
				[this, &textures, &wrapToString]()
				{
					for (const auto& [wrap, wrapStr] : wrapToString)
					{
						if (ImGui::Selectable(wrapStr.c_str(), textures[0]->Wrap() == wrap))
						{
							for (auto& tex : textures)
							{
								tex->SetWrap(wrap);
							}
						}
					}
				}
			);

			static int32_t* idOfInterest = nullptr;
			std::shared_ptr<Texture> texture = AssetManager::GetTexture(material.AlbedoTextureID);
			if (ImGui::TextureFrame("##Albedo", (ImTextureID)texture->GetID(),
				[this, &texture, &material]()
				{
					ImGui::Text("Diffuse texture");
					ImGui::Text(texture->Name().c_str());
					ImGui::ColorEdit4("Color", glm::value_ptr(material.Color), ImGuiColorEditFlags_NoInputs);
					ImGui::Checkbox("Emissive", &material.Emissive);

					if (material.Emissive)
					{
						ImGui::PrettyDragFloat("Emission", &material.Emission, 0.001f, 0.0f, FLT_MAX, "%.3f", 70.0f);
					}
				}
			))
			{
				idOfInterest = &material.AlbedoTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			texture = AssetManager::GetTexture(material.NormalTextureID);
			if (ImGui::TextureFrame("##Normal", (ImTextureID)texture->GetID(),
				[this, &texture, &material]()
				{
					ImGui::Text("Normal map");
					ImGui::Text(texture->Name().c_str());
				}
			))
			{
				idOfInterest = &material.NormalTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			texture = AssetManager::GetTexture(material.HeightTextureID);
			if (ImGui::TextureFrame("##Height", (ImTextureID)texture->GetID(),
				[this, &texture, &material]()
				{
					ImGui::Text("Height map");
					ImGui::Text(texture->Name().c_str());
					ImGui::PrettyDragFloat("Height", &material.HeightFactor, 0.001f, 0.0f, FLT_MAX, "%.3f", 70.0f);
					ImGui::Checkbox("Depth map", &material.IsDepthMap);
				}
			))
			{
				idOfInterest = &material.HeightTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			texture = AssetManager::GetTexture(material.RoughnessTextureID);
			if (ImGui::TextureFrame("##Roughness", (ImTextureID)texture->GetID(),
				[this, &texture, &material]()
				{
					ImGui::Text("Roughness map");
					ImGui::Text(texture->Name().c_str());
					ImGui::PrettyDragFloat("Roughness", &material.RoughnessFactor, 0.001f, 0.0f, 1.0f, "%.3f", 70.0f);
				}
			))
			{
				idOfInterest = &material.RoughnessTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			texture = AssetManager::GetTexture(material.MetallicTextureID);
			if (ImGui::TextureFrame("##Metallic", (ImTextureID)texture->GetID(),
				[this, &texture, &material]()
				{
					ImGui::Text("Metallic map");
					ImGui::Text(texture->Name().c_str());
					ImGui::PrettyDragFloat("Metallic", &material.MetallicFactor, 0.001f, 0.0f, 1.0f, "%.3f", 70.0f);
				}
			))
			{
				idOfInterest = &material.MetallicTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			texture = AssetManager::GetTexture(material.AmbientOccTextureID);
			if (ImGui::TextureFrame("##AO", (ImTextureID)texture->GetID(),
				[this, &texture, &material]()
				{
					ImGui::Text("AO map");
					ImGui::Text(texture->Name().c_str());
					ImGui::PrettyDragFloat("AO", &material.AmbientOccFactor, 0.001f, 0.0f, 1.0f, "%.3f", 70.0f);
				}
			))
			{
				idOfInterest = &material.AmbientOccTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10.0f, 10.0f });
			if (ImGui::BeginPopup("available_textures_group"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 15.0f, 0.0f });
				
				uint8_t cnt = 1;
				for (const auto& [id, texture] : AssetManager::AllTextures())
				{
					if (ImGui::ImageButton((ImTextureID)(texture->GetID()), { 64.0f, 64.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
					{
						*idOfInterest = id;
					}

					if (cnt++ % 3 == 0)
					{
						ImGui::NewLine();
					}
					else
					{
						ImGui::SameLine();
					}
				}

				ImGui::PopStyleVar();
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 10.0f });

				if (cnt % 3 != 1)
				{
					ImGui::NewLine();
				}

				float width = ImGui::GetContentRegionAvail().x;
				float textWidth = ImGui::CalcTextSize("Add new texture").x;
				
				ImGui::SetCursorPosX(width / 2.0f - textWidth / 2.0f + 6.5f);
				if (ImGui::PrettyButton("Add new texture"))
				{
					std::optional<std::string> path = OpenFileDialog(std::filesystem::current_path().string());

					if (path.has_value())
					{
						std::shared_ptr<Texture> texture = std::make_shared<Texture>(path.value());
						*idOfInterest = AssetManager::AddTexture(texture);
						ImGui::CloseCurrentPopup();
					}
				}
				
				ImGui::PopStyleVar();
				ImGui::EndPopup();
			}
			ImGui::PopStyleVar();

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<MaterialComponent>();
			}

			ImGui::Unindent(16.0f);
		}
	}
	ImGui::PopID();

	ImGui::PushID(4);
	if (m_SelectedEntity.HasComponent<DirectionalLightComponent>())
	{
		DirectionalLightComponent& light = m_SelectedEntity.GetComponent<DirectionalLightComponent>();

		if (ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::PrettyDragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<DirectionalLightComponent>();
			}

			ImGui::Unindent(16.0f);
		}
	}
	ImGui::PopID();

	ImGui::PushID(5);
	if (m_SelectedEntity.HasComponent<PointLightComponent>())
	{
		PointLightComponent& light = m_SelectedEntity.GetComponent<PointLightComponent>();

		if (ImGui::CollapsingHeader("Point light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::PrettyDragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");
			ImGui::PrettyDragFloat("Linear", &light.LinearTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f");
			ImGui::PrettyDragFloat("Quadratic", &light.QuadraticTerm, 0.00001f, 0.0f, FLT_MAX, "%.5f");

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<PointLightComponent>();
			}

			ImGui::Unindent(16.0f);
		}
	}
	ImGui::PopID();

	ImGui::PushID(6);
	if (m_SelectedEntity.HasComponent<SpotLightComponent>())
	{
		SpotLightComponent& light = m_SelectedEntity.GetComponent<SpotLightComponent>();

		if (ImGui::CollapsingHeader("Spotlight", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::PrettyDragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");
			ImGui::PrettyDragFloat("Cutoff", &light.Cutoff, 0.01f, 0.0f, FLT_MAX, "%.3f");
			ImGui::PrettyDragFloat("Smoothness", &light.EdgeSmoothness, 0.01f, 0.0f, light.Cutoff, "%.3f");
			ImGui::PrettyDragFloat("Linear", &light.LinearTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f");
			ImGui::PrettyDragFloat("Quadratic", &light.QuadraticTerm, 0.00001f, 0.0f, FLT_MAX, "%.5f");

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<SpotLightComponent>();
			}

			ImGui::Unindent(16.0f);
		}
	}
	ImGui::PopID();

	ImGui::Separator();
	ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - ImGui::CalcTextSize("Add new component").x / 2.0f);

	if (ImGui::PrettyButton("Add new component"))
	{
		ImGui::OpenPopup("add_component_group");
	}

	if (ImGui::BeginPopup("add_component_group"))
	{
		auto [width, height] = ImGui::CalcTextSize("Directional light");
		width += ImGui::GetStyle().FramePadding.x * 2.0f;
		height += ImGui::GetStyle().FramePadding.y * 2.0f;

		if (!m_SelectedEntity.HasComponent<MaterialComponent>() && ImGui::PrettyButton("Material", { width, height }))
		{
			m_SelectedEntity.AddComponent<MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<MeshComponent>() && ImGui::PrettyButton("Mesh", { width, height }))
		{
			m_SelectedEntity.AddComponent<MeshComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<DirectionalLightComponent>() && ImGui::PrettyButton("Directional light", { width, height }))
		{
			m_SelectedEntity.AddComponent<DirectionalLightComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<PointLightComponent>() && ImGui::PrettyButton("Point light", { width, height }))
		{
			m_SelectedEntity.AddComponent<PointLightComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<SpotLightComponent>() && ImGui::PrettyButton("Spotlight", { width, height }))
		{
			m_SelectedEntity.AddComponent<SpotLightComponent>();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::End();
}

void EditorLayer::RenderGizmo()
{
	if (m_SelectedEntity.Handle() == entt::null || m_GizmoOp == -1)
	{
		m_LockFocus = false;
		m_IsGizmoUsed = false;
		return;
	}

	const WindowSpec& spec = Application::Instance()->Spec();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect((float)Application::Instance()->Spec().Width * 0.2f, 0.0f, spec.Width * 0.6f, spec.Height);

	const glm::mat4& cameraProj = m_EditorCamera.GetProjection();
	const glm::mat4& cameraView = m_EditorCamera.GetViewMatrix();
	glm::mat4 transform = m_SelectedEntity.GetComponent<TransformComponent>().ToMat4();

	bool doSnap = Input::IsKeyPressed(Key::LeftControl);
	float snapStep = (m_GizmoOp == ImGuizmo::ROTATE ? 45.0f : 0.5f);
	float snapArr[3]{ snapStep, snapStep, snapStep };

	ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProj), ImGuizmo::OPERATION(m_GizmoOp),
		ImGuizmo::MODE(m_GizmoMode), glm::value_ptr(transform), nullptr, doSnap ? snapArr : nullptr);
	m_LockFocus = ImGuizmo::IsOver();
	m_IsGizmoUsed = ImGuizmo::IsUsing();

	if (m_IsGizmoUsed)
	{
		glm::vec3 translation{};
		glm::vec3 rotation{};
		glm::vec3 scale{};
		TransformComponent& transformComp = m_SelectedEntity.GetComponent<TransformComponent>();

		TransformDecompose(transform, translation, rotation, scale);
		glm::vec3 deltaRotation = rotation - transformComp.Rotation;
		
		transformComp.Position = translation;
		transformComp.Rotation = transformComp.Rotation + deltaRotation;
		transformComp.Scale = scale;
	}
}
