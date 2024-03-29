#include "EditorLayer.hpp"

#include "../Application.hpp"
#include "../Timer.hpp"
#include "../renderer/Renderer.hpp"
#include "../Logger.hpp"
#include "../scenes/Entity.hpp"
#include "../renderer/AssetManager.hpp"
#include "../Math.hpp"

#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

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

	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/arrow.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/cbbl.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/plank.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/sand.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/sand.png"));
	AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/grass.jpg"));

	Entity nothing = m_Scene.SpawnEntity("Nothing");
	nothing.GetComponent<TransformComponent>().Position = { 0.0f, 2.0f, 0.0f };
	nothing.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_CUBE;
	nothing.AddComponent<MaterialComponent>().AlbedoTextureID = AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/glass.png"));
	nothing.AddComponent<PointLightComponent>();

	Entity ground = m_Scene.SpawnEntity("Ground");
	ground.GetComponent<TransformComponent>().Scale = { 50.0f, 0.1f, 50.0f };
	ground.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_CUBE;
	ground.AddComponent<MaterialComponent>().AlbedoTextureID = AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/brickwall.jpg"));
	ground.GetComponent<MaterialComponent>().NormalTextureID = AssetManager::AddTexture(std::make_shared<Texture>("resources/textures/brickwall_normal.jpg"));
	ground.GetComponent<MaterialComponent>().TilingFactor = { 20.0f, 20.0f };
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
		
		if (m_EditorCamera.ControlType() != CameraControlType::FirstPersonControl)
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
		uint32_t width = (uint32_t)(ev.Size.Width * 0.8f);
		uint32_t height = ev.Size.Height;

		Renderer::OnWindowResize({ 0, 0, (int32_t)(ev.Size.Width * 0.8f), ev.Size.Height });
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

	if (ImGui::Button("New entity"))
	{
		ImGui::OpenPopup("new_entity_group");
	}

	if (ImGui::BeginPopup("new_entity_group"))
	{
		auto [width, height] = ImGui::CalcTextSize("Empty entity");
		width += ImGui::GetStyle().FramePadding.x * 2.0f;
		height += ImGui::GetStyle().FramePadding.y * 2.0f;

		if (ImGui::Button("Empty entity", { width, height }))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Empty entity");
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Button("Plane", { width, height }))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Plane");
			m_SelectedEntity.GetComponent<TransformComponent>().Rotation = { glm::radians(-90.0f), 0.0f, 0.0f };
			m_SelectedEntity.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_PLANE;
			m_SelectedEntity.AddComponent<MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Button("Cube", { width, height }))
		{
			m_SelectedEntity = m_Scene.SpawnEntity("Cube");
			m_SelectedEntity.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_CUBE;
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

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(16.0f);
		ImGui::DragFloat3("Position", glm::value_ptr(m_EditorCamera.Position), 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Exposure", &m_EditorCamera.Exposure, 0.001f, 0.0f, 5.0f);
		ImGui::DragFloat("Pitch", &m_EditorCamera.m_Pitch, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::DragFloat("Yaw", &m_EditorCamera.m_Yaw, 1.0f, -FLT_MAX, FLT_MAX);
		ImGui::Unindent(16.0f);
	}

	ImGui::NewLine();
	float width = ImGui::GetContentRegionAvail().x;
	ImGui::BeginChild("Picker", { width, width }, true);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Image((ImTextureID)m_ScreenFB->GetColorAttachmentID(1), ImGui::GetContentRegionAvail(), { 0.0f, 1.0f }, { 1.0f, 0.0f });
	ImGui::PopStyleVar();
	ImGui::EndChild();

	ImGui::End();
}

void EditorLayer::RenderViewport()
{
	m_MainFB->Bind();
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
	Renderer::SetStencilFunc(GL_ALWAYS, 0, 0xFF);
	Renderer::SetStencilMask(0x00);
	m_MainFB->ClearColorAttachment(1);
	m_Scene.Render(m_EditorCamera);

	// Render selected entity outline
	if (m_SelectedEntity.Handle() != entt::null && m_SelectedEntity.HasComponent<MeshComponent>())
	{
		TransformComponent  transform = m_SelectedEntity.GetComponent<TransformComponent>();
		MeshComponent mesh = m_SelectedEntity.GetComponent<MeshComponent>();
		MaterialComponent material{};

		transform.Scale += 0.2f;
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
		ImGui::DragFloat3("Position", glm::value_ptr(transform.Position), 0.05f);
		ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.05f);
		ImGui::DragFloat3("Scale",    glm::value_ptr(transform.Scale),    0.05f);
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
			if (ImGui::BeginCombo("##MeshCombo", AssetManager::GetMesh(mesh.MeshID).MeshName.c_str()))
			{
				const std::unordered_map<int32_t, Mesh>& meshes = AssetManager::AllMeshes();

				for (const auto& [id, meshData] : meshes)
				{
					if (ImGui::Selectable(meshData.MeshName.c_str(), id == mesh.MeshID))
					{
						mesh.MeshID = id;
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::Button("Remove component"))
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
		MaterialComponent& material = m_SelectedEntity.GetComponent<MaterialComponent>();

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(16.0f);
			ImGui::ColorEdit4("Color", glm::value_ptr(material.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::DragFloat2("Tiling factor", glm::value_ptr(material.TilingFactor), 0.01f, 0.0f, FLT_MAX);
			ImGui::DragFloat2("Texture offset", glm::value_ptr(material.TextureOffset), 0.01f, -FLT_MAX, FLT_MAX);
			ImGui::DragFloat("Shininess", &material.Shininess, 0.1f, 0.0f, 128.0f);

			std::shared_ptr<Texture> tex    = AssetManager::GetTexture(material.AlbedoTextureID);
			std::shared_ptr<Texture> normal = AssetManager::GetTexture(material.NormalTextureID);
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

			if (ImGui::ImageButton((ImTextureID)(AssetManager::GetTexture(material.AlbedoTextureID)->GetID()), { 64.0f, 64.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.AlbedoTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			ImGui::SameLine();

			if (ImGui::ImageButton((ImTextureID)(AssetManager::GetTexture(material.NormalTextureID)->GetID()), { 64.0f, 64.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
			{
				idOfInterest = &material.NormalTextureID;
				ImGui::OpenPopup("available_textures_group");
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10.0f, 10.0f });
			if (ImGui::BeginPopup("available_textures_group"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 10.0f });

				bool newLine = false;
				for (const auto& [id, texture] : AssetManager::AllTextures())
				{
					if (ImGui::ImageButton((ImTextureID)(texture->GetID()), { 64.0f, 64.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }))
					{
						*idOfInterest = id;
					}

					if (!newLine)
					{
						ImGui::SameLine();
					}

					newLine = !newLine;
				}

				ImGui::PopStyleVar();
				ImGui::EndPopup();
			}
			ImGui::PopStyleVar();

			if (ImGui::Button("Remove component"))
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
			ImGui::DragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");

			if (ImGui::Button("Remove component"))
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
			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::DragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");
			ImGui::DragFloat("Linear attenuation", &light.LinearTerm, 0.001f, 0.0f, FLT_MAX, "%.5f");
			ImGui::DragFloat("Quadratic attenuation", &light.QuadraticTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f");

			if (ImGui::Button("Remove component"))
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
			ImGui::Indent(16.0f);
			ImGui::ColorEdit3("Color", glm::value_ptr(light.Color), ImGuiColorEditFlags_NoInputs);
			ImGui::DragFloat("Intensity", &light.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");
			ImGui::DragFloat("Cutoff", &light.Cutoff, 0.01f, 0.0f, FLT_MAX, "%.3f");
			ImGui::DragFloat("Edge smoothness", &light.EdgeSmoothness, 0.01f, 0.0f, light.Cutoff, "%.3f");
			ImGui::DragFloat("Linear attenuation", &light.LinearTerm, 0.001f, 0.0f, FLT_MAX, "%.5f");
			ImGui::DragFloat("Quadratic attenuation", &light.QuadraticTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f");

			if (ImGui::Button("Remove component"))
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

	if (ImGui::Button("Add new component"))
	{
		ImGui::OpenPopup("add_component_group");
	}

	if (ImGui::BeginPopup("add_component_group"))
	{
		auto [width, height] = ImGui::CalcTextSize("Directional light");
		width += ImGui::GetStyle().FramePadding.x * 2.0f;
		height += ImGui::GetStyle().FramePadding.y * 2.0f;

		if (!m_SelectedEntity.HasComponent<MaterialComponent>() && ImGui::Button("Material", { width, height }))
		{
			m_SelectedEntity.AddComponent<MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<MeshComponent>() && ImGui::Button("Mesh", { width, height }))
		{
			m_SelectedEntity.AddComponent<MeshComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<DirectionalLightComponent>() && ImGui::Button("Directional light", { width, height }))
		{
			m_SelectedEntity.AddComponent<DirectionalLightComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<PointLightComponent>() && ImGui::Button("Point light", { width, height }))
		{
			m_SelectedEntity.AddComponent<PointLightComponent>();
			ImGui::CloseCurrentPopup();
		}

		if (!m_SelectedEntity.HasComponent<SpotLightComponent>() && ImGui::Button("Spotlight", { width, height }))
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

		Math::TransformDecompose(transform, translation, rotation, scale);
		glm::vec3 deltaRotation = rotation - transformComp.Rotation;
		
		transformComp.Position = translation;
		transformComp.Rotation = transformComp.Rotation + deltaRotation;
		transformComp.Scale = scale;
	}
}
