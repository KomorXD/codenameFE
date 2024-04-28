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

MaterialEditLayer::MaterialEditLayer(std::vector<Entity>& mainEntities)
	: m_Camera(90.0f, 16.0f / 9.0f, 0.1f, 1000.0f)
	, m_MainEntities(mainEntities)
{
	FUNC_PROFILE();

	const WindowSpec& spec = Application::Instance()->Spec();
	Event dummyEv{};
	dummyEv.Type = Event::WindowResized;
	dummyEv.Size.Width = spec.Width * 0.8f;
	dummyEv.Size.Height = spec.Height;

	m_Camera.OnEvent(dummyEv);
	m_Camera.Position = { 0.0f, 0.0f, -3.0f };

	Renderer::OnWindowResize({ 0, 0, (int32_t)(spec.Width * 0.8f), spec.Height });

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

	m_SampleEnt = m_Scene.SpawnEntity("Sphere");
	m_SampleEnt.AddComponent<MeshComponent>().MeshID = AssetManager::MESH_SPHERE;
	m_SampleEnt.AddComponent<MaterialComponent>();

	m_Camera.SetControls(std::make_unique<OrbitalControls>(&m_Camera, &m_SampleEnt.GetComponent<TransformComponent>().Position));

	m_LightEnt = m_Scene.SpawnEntity("Light source");
	m_LightEnt.GetComponent<TransformComponent>().Position = { -1.5f, 1.5f, -2.0f };
	m_LightEnt.AddComponent<PointLightComponent>();
}

MaterialEditLayer::MaterialEditLayer(std::vector<Entity>& mainEntities, int32_t materialID)
	: MaterialEditLayer(mainEntities)
{
	m_SampleEnt.GetComponent<MaterialComponent>().MaterialID = materialID;
}

void MaterialEditLayer::OnAttach()
{
	FUNC_PROFILE();
}

void MaterialEditLayer::OnEvent(Event& ev)
{
	if (ev.Type == Event::WindowResized)
	{
		uint32_t width = (uint32_t)(ev.Size.Width * 0.8f);
		uint32_t height = ev.Size.Height;

		Renderer::OnWindowResize({ 0, 0, (int32_t)(ev.Size.Width * 0.8f), ev.Size.Height });
		m_Camera.OnEvent(ev);

		m_ScreenFB->Bind();
		m_ScreenFB->ResizeEverything({ width, height });
		m_ScreenFB->Unbind();

		m_MainFB->Bind();
		m_MainFB->ResizeEverything({ width, height });
		m_MainFB->Unbind();

		return;
	}

	m_Camera.OnEvent(ev);
}

void MaterialEditLayer::OnUpdate(float ts)
{
	glm::vec3 normalizedCameraForward = glm::normalize(m_Camera.GetForwardDirection());
	float distance = glm::length(m_Camera.Position);
	m_Camera.Position = -normalizedCameraForward * distance;
	m_Camera.OnUpdate(ts);
}

void MaterialEditLayer::OnTick()
{
}

void MaterialEditLayer::OnRender()
{
	RenderPanel();
	
	m_MainFB->Bind();
	m_MainFB->BindRenderbuffer();
	m_MainFB->DrawToColorAttachment(0, 0);
	m_MainFB->DrawToColorAttachment(1, 1);
	m_MainFB->FillDrawBuffers();
	Renderer::ClearColor(glm::vec4(m_BgColor, 1.0f));
	Renderer::Clear();
	m_MainFB->ClearColorAttachment(1);
	m_Scene.Render(m_Camera);
	m_MainFB->DrawToColorAttachment(0, 0);
	m_MainFB->DrawToColorAttachment(1, 1);
	m_ScreenFB->DrawToColorAttachment(0, 0);
	m_ScreenFB->DrawToColorAttachment(1, 1);
	m_MainFB->BlitBuffers(0, 0, *m_ScreenFB);
	m_MainFB->BlitBuffers(1, 1, *m_ScreenFB);
	m_ScreenFB->Bind();
	m_ScreenFB->BindColorAttachment(0);
	m_ScreenFB->DrawToColorAttachment(2, 2);
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT2));
	Renderer::DrawScreenQuad();
	m_ScreenFB->Unbind();
	
	ImGui::SetNextWindowPos({ ((float)Application::Instance()->Spec().Width * 0.2f), 0.0f });
	ImGui::SetNextWindowSize({ (float)Application::Instance()->Spec().Width * 0.8f, (float)Application::Instance()->Spec().Height });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::Image((ImTextureID)m_ScreenFB->GetColorAttachmentID(2), ImGui::GetContentRegionAvail(), { 0.0f, 1.0f }, { 1.0f, 0.0f });
	m_ViewportHovered = ImGui::IsWindowHovered();
	m_ViewportFocused = ImGui::IsWindowFocused();
	ImGui::PopStyleVar();
	ImGui::End();
}

void MaterialEditLayer::RenderPanel()
{
	glm::vec2 windowSize = { Application::Instance()->Spec().Width, Application::Instance()->Spec().Height };
	ImGui::SetNextWindowPos({ 0.0f, 0.0f });
	ImGui::SetNextWindowSize({ (float)Application::Instance()->Spec().Width * 0.2f, (float)Application::Instance()->Spec().Height });
	ImGui::Begin("Material editor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	int32_t entityMatID = m_SampleEnt.GetComponent<MaterialComponent>().MaterialID;
	ImVec2 avSpace = ImGui::GetContentRegionAvail();
	ImGui::Text("Registered materials");
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	ImGui::BeginChild("Materials", ImVec2(avSpace.x, avSpace.y / 5.0f), true);
	for (const auto& [id, mat] : AssetManager::AllMaterials())
	{
		ImGui::PushID(id);
		if (ImGui::Selectable(mat.Name.c_str(), entityMatID == id))
		{
			m_SampleEnt.GetComponent<MaterialComponent>().MaterialID = id;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	float buttonWidth = ImGui::GetContentRegionAvail().x / 3.0f;
	ImVec2 buttonSize(buttonWidth, 0.0f);
	ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - buttonWidth);
	if (ImGui::PrettyButton("Go back", buttonSize))
	{
		Application::Instance()->PopLayer();
	}
	ImGui::SameLine();
	if (ImGui::PrettyButton("New material", buttonSize))
	{
		Material mat{};
		mat.Name = "New material";
		m_SampleEnt.GetComponent<MaterialComponent>().MaterialID = AssetManager::AddMaterial(mat);
	}


	if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(16.0f);
		ImGui::Text("General");
		ImGui::Separator();
		ImGui::ColorEdit3("Sky color", glm::value_ptr(m_BgColor));

		MeshComponent& mc = m_SampleEnt.GetComponent<MeshComponent>();
		ImGui::BeginPrettyCombo("Mesh", AssetManager::GetMesh(mc.MeshID).Name.c_str(),
			[this, &mc]()
			{
				const std::unordered_map<int32_t, Mesh>& meshes = AssetManager::AllMeshes();

				for (const auto& [id, meshData] : meshes)
				{
					if (ImGui::Selectable(meshData.Name.c_str(), id == mc.MeshID))
					{
						mc.MeshID = id;
					}
				}
			}
		);

		TransformComponent& tc = m_LightEnt.GetComponent<TransformComponent>();
		PointLightComponent& plc = m_LightEnt.GetComponent<PointLightComponent>();
		ImGui::NewLine();
		ImGui::Text("Light");
		ImGui::Separator();
		ImGui::PrettyDragFloat3("Position", glm::value_ptr(tc.Position), 0.05f);

		ImGui::PushID(1);
		ImGui::ColorEdit3("Color", glm::value_ptr(plc.Color), ImGuiColorEditFlags_NoInputs);
		ImGui::PopID();

		ImGui::PrettyDragFloat("Intensity", &plc.Intensity, 0.001f, 0.0f, FLT_MAX, "%.3f");
		ImGui::PrettyDragFloat("Linear", &plc.LinearTerm, 0.0001f, 0.0f, FLT_MAX, "%.5f");
		ImGui::PrettyDragFloat("Quadratic", &plc.QuadraticTerm, 0.00001f, 0.0f, FLT_MAX, "%.5f");
		ImGui::Unindent(16.0f);
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Material& material = AssetManager::GetMaterial(entityMatID);
		char buf[64];
		strcpy_s(buf, 64, material.Name.data());

		ImGui::Indent(16.0f);
		ImGui::PrettyInputText("Name", buf);
		material.Name = buf;
		// ImGui::InputText("Name", buf, 64, ImGuiInputTextFlags_EnterReturnsTrue);
		// material.Name = buf;

		ImGui::PrettyDragFloat2("Tiling factor", glm::value_ptr(material.TilingFactor), 0.01f, 0.0f, FLT_MAX);
		ImGui::PrettyDragFloat2("Texture offset", glm::value_ptr(material.TextureOffset), 0.01f, -FLT_MAX, FLT_MAX);

		std::shared_ptr<Texture> tex    = AssetManager::GetTexture(material.AlbedoTextureID);
		std::shared_ptr<Texture> normal = AssetManager::GetTexture(material.NormalTextureID);

		ImGui::BeginPrettyCombo("Filtering", tex->Filter() == GL_NEAREST ? "Nearest" : "Linear",
			[this, &tex, &normal]()
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
			}
		);

		const std::unordered_map<int32_t, std::string> wrapToString = {
			{ GL_CLAMP_TO_EDGE,		   "Clamp to edge"		  },
			{ GL_CLAMP_TO_BORDER,	   "Clamp to border"	  },
			{ GL_REPEAT,			   "Repeat"				  },
			{ GL_MIRROR_CLAMP_TO_EDGE, "Mirror clamp to edge" },
			{ GL_MIRRORED_REPEAT,	   "Mirrorer repeat"	  }
		};
		std::string preview = wrapToString.at(tex->Wrap());

		ImGui::BeginPrettyCombo("Wrapping", preview.c_str(),
			[this, &tex, &normal, &wrapToString]()
			{
				for (const auto& [wrap, wrapStr] : wrapToString)
				{
					if (ImGui::Selectable(wrapStr.c_str(), tex->Filter() == wrap))
					{
						tex->SetWrap(wrap);
						normal->SetWrap(wrap);
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

		if (entityMatID != AssetManager::MATERIAL_DEFAULT && ImGui::PrettyButton("Remove material"))
		{
			AssetManager::RemoveMaterial(entityMatID);
			m_SampleEnt.GetComponent<MaterialComponent>().MaterialID = AssetManager::MATERIAL_DEFAULT;

			for (Entity& ent : m_MainEntities)
			{
				if (ent.HasComponent<MaterialComponent>())
				{
					MaterialComponent& mc = ent.GetComponent<MaterialComponent>();
					if (mc.MaterialID == entityMatID)
					{
						mc.MaterialID = AssetManager::MATERIAL_DEFAULT;
					}
				}
			}
		}

		ImGui::Unindent(16.0f);
		ImGui::NewLine();
	}

	ImGui::End();
}
