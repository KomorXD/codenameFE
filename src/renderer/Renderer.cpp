#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Renderer.hpp"
#include "../Logger.hpp"
#include "../Timer.hpp"
#include "Camera.hpp"
#include "PrimitivesGen.hpp"
#include "AssetManager.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static void OpenGLMessageCallback(
	unsigned source,
	unsigned type,
	unsigned id,
	unsigned severity,
	int length,
	const char* message,
	const void* userParam
)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:		 LOG_CRITICAL(message); return;
	case GL_DEBUG_SEVERITY_MEDIUM:		 LOG_ERROR(message);	return;
	case GL_DEBUG_SEVERITY_LOW:			 LOG_WARN(message);		return;
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO(message);		return;
	}
}

Camera* Renderer::s_ActiveCamera = nullptr;
Viewport Renderer::s_Viewport;

struct ScreenQuadVertex
{
	glm::vec2 Position;
	glm::vec2 TextureUV;
};

struct LineVertex
{
	glm::vec3 Position;
	glm::vec4 Color;
};

struct MeshInstance
{
	glm::mat4 Transform;
	glm::vec4 Color;
	glm::vec2 TilingFactor;
	glm::vec2 TextureOffset;

	float TextureSlot;
	float NormalTextureSlot;
	float EntityID;
};

struct MeshBufferData
{
	int32_t CurrentInstancesCount = 0;
	std::vector<MeshInstance> Instances;
};

struct RendererData
{
	static constexpr uint32_t MaxQuads	   = 5000;
	static constexpr uint32_t MaxVertices  = MaxQuads * 4;
	static constexpr uint32_t MaxIndices   = MaxQuads * 6;
	static constexpr uint32_t MaxInstances = MaxQuads / 4;
	static constexpr uint32_t MaxInstancesOfType = MaxInstances / 5;

	uint32_t InstancesCount = 0;
	std::unordered_map<int32_t, MeshBufferData> MeshesData;

	std::shared_ptr<VertexArray>  ScreenQuadVertexArray;
	std::shared_ptr<VertexBuffer> ScreenQuadVertexBuffer;
	std::shared_ptr<Shader>		  ScreenQuadShader;

	std::shared_ptr<VertexArray>  LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;
	std::shared_ptr<Shader>		  LineShader;
	uint32_t	LineVertexCount = 0;
	LineVertex* LineBufferBase  = nullptr;
	LineVertex* LineBufferPtr   = nullptr;

	std::shared_ptr<Shader>	DefaultShader;
	std::shared_ptr<Shader> PickerShader;
	std::shared_ptr<Shader> CurrentShader;
	
	uint32_t DirLightsCount   = 0;
	uint32_t PointLightsCount = 0;
	uint32_t SpotLightsCount  = 0;

	std::shared_ptr<UniformBuffer> MatricesBuffer;

	uint32_t BoundTexturesCount = 1;
	std::array<int32_t, 24> TextureBindings;
	
};

static RendererData s_Data{};

static Mesh GenerateMeshData(VertexData vertexData)
{
	Mesh mesh{};
	auto& [vertices, indices] = vertexData;
	std::unique_ptr<IndexBuffer> ibo = std::make_unique<IndexBuffer>(indices.data(), (uint32_t)indices.size());
	mesh.VAO = std::make_shared<VertexArray>();
	mesh.VBO = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex), vertices.size());

	VertexBufferLayout layout;
	layout.Push<float>(3); // 0 Position
	layout.Push<float>(3); // 1 Normal
	layout.Push<float>(3); // 2 Tangent
	layout.Push<float>(3); // 3 Bitangent
	layout.Push<float>(2); // 4 Texture UV
	mesh.VAO->AddBuffers(mesh.VBO, ibo, layout);

	layout.Clear();
	layout.Push<float>(4); // 5  Transform
	layout.Push<float>(4); // 6  Transform
	layout.Push<float>(4); // 7  Transform
	layout.Push<float>(4); // 8  Transform
	layout.Push<float>(4); // 9  Color
	layout.Push<float>(2); // 10 Tiling factor
	layout.Push<float>(2); // 11 Texture offset
	layout.Push<float>(1); // 12 Texture slot
	layout.Push<float>(1); // 13 Normal texture slot
	layout.Push<float>(1); // 14 Entity ID
	mesh.InstanceBuffer = std::make_shared<VertexBuffer>(nullptr, s_Data.MaxInstancesOfType * sizeof(MeshInstance));
	mesh.VAO->AddInstancedVertexBuffer(mesh.InstanceBuffer, layout, 5);

	return mesh;
}

void Renderer::Init()
{
	FUNC_PROFILE();

#ifdef CONF_DEBUG
	GLCall(glEnable(GL_DEBUG_OUTPUT));
	GLCall(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
	GLCall(glDebugMessageCallback(OpenGLMessageCallback, nullptr));
	GLCall(glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE));
#endif

	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glDepthFunc(GL_LESS));
	GLCall(glEnable(GL_BLEND));
	GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GLCall(glEnable(GL_CULL_FACE));
	GLCall(glCullFace(GL_BACK));
	GLCall(glEnable(GL_LINE_SMOOTH));
	GLCall(glEnable(GL_MULTISAMPLE));
	GLCall(glEnable(GL_STENCIL_TEST));
	GLCall(glStencilFunc(GL_NOTEQUAL, 1, 0xFF));
	GLCall(glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE));
	GLCall(glStencilMask(0x00));

	{
		SCOPE_PROFILE("Quad mesh init");

		Mesh quadMesh = GenerateMeshData(QuadMeshData());
		quadMesh.MeshName = "Quad";

		int32_t meshID = AssetManager::AddMesh(quadMesh, AssetManager::MESH_PLANE);
		s_Data.MeshesData[meshID].Instances.reserve(s_Data.MaxInstancesOfType);
	}

	{
		SCOPE_PROFILE("Cube mesh init");

		Mesh cubeMesh = GenerateMeshData(CubeMeshData());
		cubeMesh.MeshName = "Cube";

		int32_t meshID = AssetManager::AddMesh(cubeMesh, AssetManager::MESH_CUBE);
		s_Data.MeshesData[meshID].Instances.reserve(s_Data.MaxInstancesOfType);
	}

	{
		SCOPE_PROFILE("Screen quad init");

		std::array<ScreenQuadVertex, 6> vertices =
		{
			ScreenQuadVertex{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) },
			ScreenQuadVertex{ glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
			ScreenQuadVertex{ glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f, 1.0f) },

			ScreenQuadVertex{ glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f, 1.0f) },
			ScreenQuadVertex{ glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 1.0f) },
			ScreenQuadVertex{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) }
		};

		s_Data.ScreenQuadVertexArray = std::make_shared<VertexArray>();
		s_Data.ScreenQuadVertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(ScreenQuadVertex));

		VertexBufferLayout layout;
		layout.Push<float>(2); // Position
		layout.Push<float>(2); // Texture UV

		s_Data.ScreenQuadVertexArray->AddVertexBuffer(s_Data.ScreenQuadVertexBuffer, layout);
		s_Data.ScreenQuadShader = std::make_shared<Shader>("resources/shaders/ScreenQuad.vert", "resources/shaders/ScreenQuad.frag");
		s_Data.ScreenQuadShader->Bind();
		s_Data.ScreenQuadShader->SetUniform1i("u_ScreenTexture", 0);
	}

	{
		SCOPE_PROFILE("Line data init");

		s_Data.LineVertexArray = std::make_shared<VertexArray>();
		s_Data.LineVertexBuffer = std::make_shared<VertexBuffer>(nullptr, s_Data.MaxVertices * sizeof(LineVertex));

		VertexBufferLayout layout;
		layout.Push<float>(3); // Position
		layout.Push<float>(4); // Color

		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer, layout);
		s_Data.LineBufferBase = new LineVertex[s_Data.MaxVertices];
		s_Data.LineShader = std::make_shared<Shader>("resources/shaders/Line.vert", "resources/shaders/Line.frag");
	}

	{
		SCOPE_PROFILE("Uniform buffers init");

		s_Data.MatricesBuffer = std::make_shared<UniformBuffer>(nullptr, 2 * sizeof(glm::mat4));
		s_Data.MatricesBuffer->BindBufferRange(0, 0, 2 * sizeof(glm::mat4));
	}

	{
		SCOPE_PROFILE("Other setup");

		s_Data.DefaultShader = std::make_shared<Shader>("resources/shaders/Default.vert", "resources/shaders/Default.frag");
		s_Data.DefaultShader->Bind();
		for (int32_t i = 0; i < s_Data.TextureBindings.size(); i++)
		{
			s_Data.DefaultShader->SetUniform1i("u_Textures[" + std::to_string(i) + "]", i);
		}

		uint8_t whitePixel[] = { 255, 255, 255, 255 };
		std::shared_ptr<Texture> defaultAlbedo = std::make_shared<Texture>(whitePixel, 1, 1);
		AssetManager::AddTexture(defaultAlbedo, AssetManager::TEXTURE_WHITE);

		uint8_t normalPixel[] = { 127, 127, 255, 255 };
		std::shared_ptr<Texture> defaultNormal = std::make_shared<Texture>(normalPixel, 1, 1);
		AssetManager::AddTexture(defaultNormal, AssetManager::TEXTURE_NORMAL);

		s_Data.PickerShader = std::make_shared<Shader>("resources/shaders/Picker.vert", "resources/shaders/Picker.frag");
		s_Data.CurrentShader = s_Data.DefaultShader;
	}
}

void Renderer::Shutdown()
{
	delete[] s_Data.LineBufferBase;
}

void Renderer::ReloadShaders()
{
	s_Data.LineShader->ReloadShader();
	s_Data.DefaultShader->ReloadShader();
}

void Renderer::OnWindowResize(const Viewport& newViewport)
{
	s_Viewport = newViewport;
	
	auto& [x, y, width, height] = newViewport;
	GLCall(glViewport(x, y, width, height));
}

void Renderer::SceneBegin(Camera& camera)
{
	s_ActiveCamera = &camera;
	glm::mat4 projection = camera.GetProjection();
	glm::mat4 view = camera.GetViewMatrix();

	s_Data.MatricesBuffer->Bind();
	s_Data.MatricesBuffer->SetData(glm::value_ptr(projection), sizeof(glm::mat4));
	s_Data.MatricesBuffer->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));

	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_ViewPos", camera.Position);

	StartBatch();
}

void Renderer::SceneEnd()
{
	Flush();
}

void Renderer::Flush()
{
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform1i("u_DirLightsCount",   s_Data.DirLightsCount);
	s_Data.DefaultShader->SetUniform1i("u_PointLightsCount", s_Data.PointLightsCount);
	s_Data.DefaultShader->SetUniform1i("u_SpotLightsCount",  s_Data.SpotLightsCount);

	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + i));
		GLCall(glBindTexture(GL_TEXTURE_2D, s_Data.TextureBindings[i]));
	}

	for (auto& [meshID, meshData] : s_Data.MeshesData)
	{
		if (meshData.CurrentInstancesCount == 0)
		{
			continue;
		}

		Mesh& mesh = AssetManager::GetMesh(meshID);
		mesh.InstanceBuffer->SetData(meshData.Instances.data(), meshData.CurrentInstancesCount * sizeof(MeshInstance));
		DrawIndexedInstanced(s_Data.CurrentShader, mesh.VAO, meshData.CurrentInstancesCount);
	}

	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineBufferPtr - (uint8_t*)s_Data.LineBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineBufferBase, dataSize);

		GLCall(glDisable(GL_CULL_FACE));
		DrawArrays(s_Data.LineShader, s_Data.LineVertexArray, s_Data.LineVertexCount, GL_LINES);
		GLCall(glEnable(GL_CULL_FACE));
	}
}

void Renderer::ClearColor(const glm::vec4& color)
{
	GLCall(glClearColor(color.r, color.g, color.b, color.a));
}

void Renderer::Clear(uint32_t bitfield)
{
	GLCall(glClear(bitfield));
}

void Renderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
	if (s_Data.LineVertexCount + 2 >= s_Data.MaxVertices)
	{
		NextBatch();
	}

	s_Data.LineBufferPtr->Position = start;
	s_Data.LineBufferPtr->Color = color;
	++s_Data.LineBufferPtr;

	s_Data.LineBufferPtr->Position = end;
	s_Data.LineBufferPtr->Color = color;
	++s_Data.LineBufferPtr;

	s_Data.LineVertexCount += 2;
}

void Renderer::DrawCube(const glm::mat4& transform, const glm::vec4& color)
{
	MeshComponent mesh;
	mesh.MeshID = AssetManager::MESH_CUBE;

	MaterialComponent material;
	material.Color = color;

	SubmitMesh(transform, mesh, material, 0);
}

void Renderer::SubmitMesh(const glm::mat4& transform, const MeshComponent& mesh, const MaterialComponent& material, int32_t entityID)
{
	if (s_Data.MeshesData[mesh.MeshID].CurrentInstancesCount >= s_Data.MaxInstancesOfType
		|| s_Data.InstancesCount >= s_Data.MaxInstances
		|| s_Data.BoundTexturesCount >= s_Data.TextureBindings.size() - 1)
	{
		NextBatch();
	}

	std::vector<MeshInstance>& instances = s_Data.MeshesData[mesh.MeshID].Instances;
	MeshInstance& instance = instances.emplace_back();
	instance.Transform = transform;
	instance.Color = material.Color;
	instance.TilingFactor = material.TilingFactor;
	instance.TextureOffset = material.TextureOffset;
	instance.EntityID = (float)entityID + 1.0f;
	
	s_Data.MeshesData[mesh.MeshID].CurrentInstancesCount++;
	s_Data.InstancesCount++;

	std::shared_ptr<Texture> albedo = AssetManager::GetTexture(material.AlbedoTextureID);
	std::shared_ptr<Texture> normal = AssetManager::GetTexture(material.NormalTextureID);
	int32_t duplicateAlbedoIdx = -1;
	int32_t duplicateNormalIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == albedo->GetID())
		{
			duplicateAlbedoIdx = i;
		}

		if (s_Data.TextureBindings[i] == normal->GetID())
		{
			duplicateNormalIdx = i;
		}
	}

	if (duplicateAlbedoIdx != -1)
	{
		instance.TextureSlot = (float)duplicateAlbedoIdx;
	}
	else
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = albedo->GetID();
		instance.TextureSlot = (float)s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	if (duplicateNormalIdx != -1)
	{
		instance.NormalTextureSlot = (float)duplicateNormalIdx;
	}
	else
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = normal->GetID();
		instance.NormalTextureSlot = (float)s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}
}

void Renderer::DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawElements(primitiveType, vao->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::DrawIndexedInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawElementsInstanced(primitiveType, vao->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr, instances));
}

void Renderer::DrawArrays(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t vertexCount, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawArrays(primitiveType, 0, vertexCount));
}

void Renderer::DrawArraysInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawArraysInstanced(primitiveType, 0, vao->VertexCount(), instances));
}

void Renderer::DrawScreenQuad()
{
	GLCall(glDisable(GL_DEPTH_TEST));
	DrawArrays(s_Data.ScreenQuadShader, s_Data.ScreenQuadVertexArray, 6);
	GLCall(glEnable(GL_DEPTH_TEST));
}

void Renderer::AddDirectionalLight(const TransformComponent& transform, const DirectionalLightComponent& light)
{
	glm::vec3 dir = glm::toMat3(glm::quat(transform.Rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);

	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_DirLights[" + std::to_string(s_Data.DirLightsCount) + "].direction", glm::normalize(dir));
	s_Data.DefaultShader->SetUniform3f("u_DirLights[" + std::to_string(s_Data.DirLightsCount) + "].color",	   light.Color);

	s_Data.DirLightsCount++;
}

void Renderer::AddPointLight(const glm::vec3& position, const PointLightComponent& light)
{
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].position",	   position);
	s_Data.DefaultShader->SetUniform3f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].color",		   light.Color);
	s_Data.DefaultShader->SetUniform1f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].linearTerm",	   light.LinearTerm);
	s_Data.DefaultShader->SetUniform1f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].quadraticTerm", light.QuadraticTerm);

	s_Data.PointLightsCount++;
}

void Renderer::AddSpotLight(const TransformComponent& transform, const SpotLightComponent& light)
{
	glm::vec3 dir = glm::toMat3(glm::quat(transform.Rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
	
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].position",	  transform.Position);
	s_Data.DefaultShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].cutOff",		  glm::cos(glm::radians(light.Cutoff)));
	s_Data.DefaultShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].direction",	  dir);
	s_Data.DefaultShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].outerCutOff",   glm::cos(glm::radians(light.OuterCutoff)));
	s_Data.DefaultShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].color",		  light.Color);
	s_Data.DefaultShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].linearTerm",	  light.LinearTerm);
	s_Data.DefaultShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].quadraticTerm", light.QuadraticTerm);

	s_Data.SpotLightsCount++;
}

void Renderer::SetBlur(bool enabled)
{
	s_Data.ScreenQuadShader->Bind();
	s_Data.ScreenQuadShader->SetUniform1i("u_BlurEnabled", enabled ? 1 : 0);
}

void Renderer::SetLight(bool enabled)
{
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniformBool("u_IsLightSource", enabled);
}

void Renderer::EnableStencil()
{
	GLCall(glEnable(GL_STENCIL_TEST));
}

void Renderer::DisableStencil()
{
	GLCall(glDisable(GL_STENCIL_TEST));
}

void Renderer::SetStencilFunc(uint32_t func, int32_t ref, uint32_t mask)
{
	GLCall(glStencilFunc(func, ref, mask));
}

void Renderer::SetStencilMask(uint32_t mask)
{
	GLCall(glStencilMask(mask));
}

void Renderer::EnableDepthTest()
{
	GLCall(glEnable(GL_DEPTH_TEST));
}

void Renderer::DisableDepthTest()
{
	GLCall(glDisable(GL_DEPTH_TEST));
}

void Renderer::EnableFaceCulling()
{
	GLCall(glEnable(GL_CULL_FACE));
}

void Renderer::DisableFaceCulling()
{
	GLCall(glDisable(GL_CULL_FACE));
}

void Renderer::DefaultRender()
{
	s_Data.CurrentShader = s_Data.DefaultShader;
}

void Renderer::PickerRender()
{
	s_Data.CurrentShader = s_Data.PickerShader;
}

Viewport Renderer::CurrentViewport()
{
	return s_Viewport;
}

void Renderer::StartBatch()
{
	s_Data.InstancesCount = 0;
	for (auto& [meshID, data] : s_Data.MeshesData)
	{
		data.CurrentInstancesCount = 0;
		data.Instances.clear();
		data.Instances.reserve(s_Data.MaxInstancesOfType);
	}

	s_Data.LineVertexCount = 0;
	s_Data.LineBufferPtr = s_Data.LineBufferBase;

	s_Data.DirLightsCount = 0;
	s_Data.PointLightsCount = 0;
	s_Data.SpotLightsCount = 0;

	s_Data.BoundTexturesCount = 1;
}

void Renderer::NextBatch()
{
	Flush();
	StartBatch();
}
