#include "EditorLayer.hpp"
#include "MaterialEditLayer.hpp"

#include "../Application.hpp"
#include "../Timer.hpp"
#include "../renderer/Renderer.hpp"
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

	const WindowSpec& spec = Application::Instance()->Spec();
	Event dummyEv{};
	dummyEv.Type = Event::WindowResized;
	dummyEv.Size.Width = spec.Width * 0.6f;
	dummyEv.Size.Height = spec.Height;

	m_EditorCamera.OnEvent(dummyEv);
	m_EditorCamera.Position = { 0.0f, 5.0f, -10.0f };

	Renderer::OnWindowResize({ 0, 0, (int32_t)(spec.Width * 0.6f), spec.Height });

	glm::uvec2 fbSize((uint32_t)(spec.Width * 0.6f), spec.Height);
	m_MainFB = std::make_unique<Framebuffer>(fbSize, 16);
	m_MainFB->AddColorAttachment(GL_RGBA16F);
	m_MainFB->AddColorAttachment(GL_RGBA8);
	m_MainFB->Unbind();

	m_ScreenFB = std::make_unique<Framebuffer>(fbSize, 1);
	m_ScreenFB->AddColorAttachment(GL_RGBA16F);
	m_ScreenFB->AddColorAttachment(GL_RGBA8);
	m_ScreenFB->AddColorAttachment(GL_RGBA16F);
	m_ScreenFB->Unbind();

	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/container_diffuse.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/container_specular.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/rustediron2_basecolor.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/rustediron2_normal.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/rustediron2_roughness.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/rustediron2_metallic.png"));

	Material mat{};
	mat.Color = { 1.0f, 0.0f, 0.0f, 1.0f };
	
	for (int32_t i = 0; i <= 10; i++)
	{
		for (int32_t j = 0; j <= 10; j++)
		{
			mat.RoughnessFactor = (float)i / 10;
			mat.MetallicFactor  = (float)j / 10;

			Entity sphere = m_Scene.SpawnEntity("Sphere #" + std::to_string(i * 10 + j));
			sphere.GetComponent<TransformComponent>().Position = glm::vec3(i * 2.5f - 10.0f, 0.0f, j * 2.5f - 10.0f);
			sphere.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_SPHERE;

			int32_t matID = AssetManager::AddMaterial(mat);
			sphere.AddComponent<MaterialComponent>().MaterialID = matID;
			AssetManager::GetMaterial(matID).Name = "Sphere red material #" + std::to_string(i * 10 + j);
		}
	}

	mat.Color = { 1.0f, 1.0f, 0.0f, 1.0f };
	Entity light = m_Scene.SpawnEntity("Light source");
	light.GetComponent<TransformComponent>().Position = { 0.0f, 10.0f, 0.0f };
	light.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_SPHERE;
	light.AddComponent<PointLightComponent>();

	int32_t matID = AssetManager::AddMaterial(mat);
	light.AddComponent<MaterialComponent>().MaterialID = matID;
	AssetManager::GetMaterial(matID).Name = "Light yellow material";
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
		m_ScreenFB->Resize({ width, height });
		m_ScreenFB->Unbind();

		m_MainFB->Bind();
		m_MainFB->Resize({ width, height });
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
	ImGui::NewLine();

	if (ImGui::PrettyButton("New entity"))
	{
		ImGui::OpenPopup("new_entity_group");
	}

	if (ImGui::PrettyButton("Material editor"))
	{
		Application::Instance()->PushLayer(std::make_unique<MaterialEditLayer>(m_Scene.m_Entities));
	}

	if (ImGui::PrettyButton("Reload shaders"))
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

	ImGui::NewLine();

	if (ImGui::BeginCombo("Gizmo mode", m_GizmoMode == ImGuizmo::WORLD ? "World" : "Local"))
	{
		if (ImGui::Selectable("World", m_GizmoMode == ImGuizmo::WORLD))
		{
			m_GizmoMode = ImGuizmo::WORLD;
		}

		if (ImGui::Selectable("Local", m_GizmoMode == ImGuizmo::LOCAL))
		{
			m_GizmoMode = ImGuizmo::LOCAL;
		}

		ImGui::EndCombo();
	}

	ImGui::NewLine();

	static bool isPBR = true;

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(16.0f);
		ImGui::PrettyDragFloat3("Position", glm::value_ptr(m_EditorCamera.Position), 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::PrettyDragFloat("Exposure", &m_EditorCamera.Exposure, 0.001f, 0.0f, 5.0f);
		ImGui::PrettyDragFloat("Pitch", &m_EditorCamera.m_Pitch, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::PrettyDragFloat("Yaw", &m_EditorCamera.m_Yaw, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::Checkbox("PBR", &isPBR);
		ImGui::Checkbox("Wireframe", &m_DrawWireframe);
		ImGui::Unindent(16.0f);

		Renderer::SetPBR(isPBR);
	}

	/*ImGui::NewLine();
	float width = ImGui::GetContentRegionAvail().x;
	ImGui::BeginChild("Picker", { width, width }, true);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Image((ImTextureID)m_ScreenFB->GetColorAttachmentID(1), ImGui::GetContentRegionAvail(), { 0.0f, 1.0f }, { 1.0f, 0.0f });
	ImGui::PopStyleVar();
	ImGui::EndChild();*/

	ImGui::End();
}

void EditorLayer::RenderViewport()
{
	m_MainFB->Bind();
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
	m_MainFB->ClearColorAttachment(1);
	m_Scene.Render(m_EditorCamera);
	Renderer::SetWireframe(false);

	// Grid
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
	
	m_MainFB->BlitBuffers(0, 0, *m_ScreenFB);
	m_MainFB->BlitBuffers(1, 1, *m_ScreenFB);
	m_ScreenFB->Bind();
	m_ScreenFB->BindColorAttachment();
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT2));
	Renderer::DrawScreenQuad();
	m_ScreenFB->Unbind();
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
	ImGui::InputText("Tag", buf, 64);
	ImGui::Unindent(16.0f);
	ImGui::NewLine();
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
		ImGui::NewLine();
	}
	ImGui::PopID();

	ImGui::PushID(2);
	if (m_SelectedEntity.HasComponent<MeshComponent>())
	{
		MeshComponent& mesh = m_SelectedEntity.GetComponent<MeshComponent>();

		if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			if (ImGui::BeginCombo("##MeshCombo", AssetManager::GetMesh(mesh.MeshID).Name.c_str()))
			{
				const std::unordered_map<int32_t, Mesh>& meshes = AssetManager::AllMeshes();

				for (const auto& [id, meshData] : meshes)
				{
					if (ImGui::Selectable(meshData.Name.c_str(), id == mesh.MeshID))
					{
						mesh.MeshID = id;
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<MeshComponent>();
			}

			ImGui::Unindent(16.0f);
			ImGui::NewLine();
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
			if (ImGui::BeginCombo("##Material", material.Name.c_str()))
			{
				for (const auto& [id, mat] : AssetManager::AllMaterials())
				{
					if (ImGui::Selectable(mat.Name.c_str(), id == m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID))
					{
						m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID = id;
					}
				}

				ImGui::EndCombo();
			}

			ImGui::SameLine();

			if (ImGui::PrettyButton("Edit"))
			{
				Application::Instance()->PushLayer(std::make_unique<MaterialEditLayer>(m_Scene.m_Entities, m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID));
			}

			ImGui::SameLine();

			if (ImGui::PrettyButton("New"))
			{
				m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID = AssetManager::AddMaterial(AssetManager::GetMaterial(AssetManager::MATERIAL_DEFAULT));
				Application::Instance()->PushLayer(std::make_unique<MaterialEditLayer>(m_Scene.m_Entities, m_SelectedEntity.GetComponent<MaterialComponent>().MaterialID));
			}

			ImGui::PrettyDragFloat2("Tiling factor", glm::value_ptr(material.TilingFactor), 0.01f, 0.0f, FLT_MAX);
			ImGui::PrettyDragFloat2("Texture offset", glm::value_ptr(material.TextureOffset), 0.01f, -FLT_MAX, FLT_MAX);

			std::shared_ptr<Texture> tex    = AssetManager::GetTexture(material.AlbedoTextureID);
			std::shared_ptr<Texture> normal = AssetManager::GetTexture(material.NormalTextureID);

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2.0f);
			if (ImGui::BeginCombo("Texture filtering", tex->Filter() == GL_NEAREST ? "Nearest" : "Linear"))
			{
				if (ImGui::Selectable("Nearest", tex->Filter() == GL_NEAREST))
				{
					tex->SetFilter(GL_NEAREST);
					normal->SetFilter(GL_NEAREST);
				}

				if (ImGui::Selectable("Linear", tex->Filter() == GL_LINEAR))
				{
					tex->SetFilter(GL_LINEAR);
					normal->SetFilter(GL_LINEAR);
				}

				ImGui::EndCombo();
			}

			const std::unordered_map<int32_t, std::string> wrapToString = {
				{ GL_CLAMP_TO_EDGE,		   "Clamp to edge"		  },
				{ GL_CLAMP_TO_BORDER,	   "Clamp to border"	  },
				{ GL_REPEAT,			   "Repeat"				  },
				{ GL_MIRROR_CLAMP_TO_EDGE, "Mirror clamp to edge" },
				{ GL_MIRRORED_REPEAT,	   "Mirrorer repeat"	  }
			};
			std::string preview = wrapToString.at(tex->Wrap());

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2.0f);
			if (ImGui::BeginCombo("Texture wrapping", preview.c_str()))
			{
				for (const auto& [wrap, wrapStr] : wrapToString)
				{
					if (ImGui::Selectable(wrapStr.c_str(), tex->Filter() == wrap))
					{
						tex->SetWrap(wrap);
						normal->SetWrap(wrap);
					}
				}

				ImGui::EndCombo();
			}

			static int32_t* idOfInterest = nullptr;
			constexpr ImVec2 IMAGE_SIZE(64.0f, 64.0f);
			constexpr float IMAGE_CELL_WIDTH = 96.0f;

			std::shared_ptr<Texture> texture = AssetManager::GetTexture(material.AlbedoTextureID);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, IMAGE_CELL_WIDTH);
			if (ImGui::ImageButton("##Albedo", (ImTextureID)(texture->GetID()), IMAGE_SIZE, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.AlbedoTextureID;
				ImGui::OpenPopup("available_textures_group");
			}
			ImGui::NextColumn();
			ImGui::Text("Diffuse texture");
			ImGui::Text(texture->Name().c_str());
			ImGui::ColorEdit4("Color", glm::value_ptr(material.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::Columns(1);

			texture = AssetManager::GetTexture(material.NormalTextureID);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, IMAGE_CELL_WIDTH);
			if (ImGui::ImageButton("##Normal", (ImTextureID)(texture->GetID()), IMAGE_SIZE, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.NormalTextureID;
				ImGui::OpenPopup("available_textures_group");
			}
			ImGui::NextColumn();
			ImGui::Text("Normal map");
			ImGui::Text(texture->Name().c_str());
			ImGui::Columns(1);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			texture = AssetManager::GetTexture(material.RoughnessTextureID);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, IMAGE_CELL_WIDTH);
			if (ImGui::ImageButton("##Roughness", (ImTextureID)(texture->GetID()), IMAGE_SIZE, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.RoughnessTextureID;
				ImGui::OpenPopup("available_textures_group");
			}
			ImGui::NextColumn();
			ImGui::Text("Roughness map");
			ImGui::Text(texture->Name().c_str());
			ImGui::DragFloat("Roughness", &material.RoughnessFactor, 0.05f, 0.0f, 1.0f);
			ImGui::Columns(1);

			texture = AssetManager::GetTexture(material.MetallicTextureID);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, IMAGE_CELL_WIDTH);
			if (ImGui::ImageButton("##Metallic", (ImTextureID)(texture->GetID()), IMAGE_SIZE, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.MetallicTextureID;
				ImGui::OpenPopup("available_textures_group");
			}
			ImGui::NextColumn();
			ImGui::Text("Metallic map");
			ImGui::Text(texture->Name().c_str());
			ImGui::DragFloat("Metallic", &material.MetallicFactor, 0.05f, 0.0f, 1.0f);
			ImGui::Columns(1);

			texture = AssetManager::GetTexture(material.AmbientOccTextureID);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, IMAGE_CELL_WIDTH);
			if (ImGui::ImageButton("##AO", (ImTextureID)(texture->GetID()), IMAGE_SIZE, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.AmbientOccTextureID;
				ImGui::OpenPopup("available_textures_group");
			}
			ImGui::NextColumn();
			ImGui::Text("AO map");
			ImGui::Text(texture->Name().c_str());
			ImGui::DragFloat("AO", &material.AmbientOccFactor, 0.05f, 0.0f, 1.0f);
			ImGui::Columns(1);

			ImGui::PopStyleColor(3);

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
			ImGui::NewLine();
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
			ImGui::NewLine();
		}
	}
	ImGui::PopID();

	ImGui::PushID(5);
	if (m_SelectedEntity.HasComponent<PointLightComponent>())
	{
		PointLightComponent& light = m_SelectedEntity.GetComponent<PointLightComponent>();

		if (ImGui::CollapsingHeader("Point light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float labelWidth = ImGui::CalcTextSize("Quadratic attenuation").x * 1.25f;

			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::PrettyDragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f", labelWidth);
			ImGui::PrettyDragFloat("Linear attenuation", &light.LinearTerm, 0.001f, 0.0f, FLT_MAX, "%.5f", labelWidth);
			ImGui::PrettyDragFloat("Quadratic attenuation", &light.QuadraticTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f", labelWidth);

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<PointLightComponent>();
			}

			ImGui::Unindent(16.0f);
			ImGui::NewLine();
		}
	}
	ImGui::PopID();

	ImGui::PushID(6);
	if (m_SelectedEntity.HasComponent<SpotLightComponent>())
	{
		SpotLightComponent& light = m_SelectedEntity.GetComponent<SpotLightComponent>();

		if (ImGui::CollapsingHeader("Spotlight", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float labelWidth = ImGui::CalcTextSize("Quadratic attenuation").x * 1.25f;

			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::PrettyDragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f", labelWidth);
			ImGui::PrettyDragFloat("Cutoff", &light.Cutoff, 0.01f, 0.0f, FLT_MAX, "%.3f", labelWidth);
			ImGui::PrettyDragFloat("Edge smoothness", &light.EdgeSmoothness, 0.01f, 0.0f, light.Cutoff, "%.3f", labelWidth);
			ImGui::PrettyDragFloat("Linear attenuation", &light.LinearTerm, 0.001f, 0.0f, FLT_MAX, "%.5f", labelWidth);
			ImGui::PrettyDragFloat("Quadratic attenuation", &light.QuadraticTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f", labelWidth);

			if (ImGui::PrettyButton("Remove component"))
			{
				m_SelectedEntity.RemoveComponent<SpotLightComponent>();
			}

			ImGui::Unindent(16.0f);
			ImGui::NewLine();
		}
	}
	ImGui::PopID();

	ImGui::Separator();
	ImGui::NewLine();

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
