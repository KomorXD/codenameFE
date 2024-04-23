#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Renderer.hpp"
#include "../Logger.hpp"
#include "../Timer.hpp"
#include "../Clock.hpp"
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
	float MaterialSlot;
	float EntityID;
};

struct MeshBufferData
{
	int32_t CurrentInstancesCount = 0;
	std::vector<MeshInstance> Instances;
};

struct DirLightBufferData
{
	glm::vec4 Direction;
	glm::vec4 Color;
};

struct PointLightBufferData
{
	glm::vec4 PosAndLinear;
	glm::vec4 ColorAndQuadratic;
};

struct SpotLightBufferData
{
	glm::vec4 PositionAndCutoff;
	glm::vec4 DirectionAndOuterCutoff;
	glm::vec4 ColorAndLinear;
	float Quadratic;
	
	glm::vec3 Padding;
};

struct MaterialsBufferData
{
	glm::vec4 Color;
	glm::vec2 TilingFactor;
	glm::vec2 TextureOffset;

	int32_t AlbedoTextureSlot;
	int32_t NormalTextureSlot;

	int32_t HeightTextureSlot;
	float HeightFactor;
	int32_t IsDepthMap;

	int32_t RoughnessTextureSlot;
	float RoughnessFactor;

	int32_t MetallicTextureSlot;
	float MetallicFactor;

	int32_t AmbientOccTextureSlot;
	float AmbientOccFactor;

	float Padding;
};

static bool MaterialToBufferCmp(const Material& lhs, const MaterialsBufferData& rhs)
{
	return lhs.Color == rhs.Color
		&& lhs.TilingFactor == rhs.TilingFactor
		&& lhs.TextureOffset == rhs.TextureOffset
		&& lhs.AlbedoTextureID == rhs.AlbedoTextureSlot
		&& lhs.NormalTextureID == rhs.NormalTextureSlot
		&& lhs.HeightTextureID == rhs.HeightTextureSlot
		&& lhs.HeightFactor == rhs.HeightFactor
		&& lhs.IsDepthMap == rhs.IsDepthMap
		&& lhs.RoughnessTextureID == rhs.RoughnessTextureSlot
		&& lhs.RoughnessFactor == rhs.RoughnessFactor
		&& lhs.MetallicTextureID == rhs.MetallicTextureSlot
		&& lhs.MetallicFactor == rhs.MetallicFactor
		&& lhs.AmbientOccTextureID == rhs.AmbientOccTextureSlot
		&& lhs.AmbientOccFactor == rhs.AmbientOccFactor;
}

static MaterialsBufferData MaterialToBuffer(const Material& material)
{
	return {
		.Color = material.Color,
		.TilingFactor = material.TilingFactor,
		.TextureOffset = material.TextureOffset,
		.HeightFactor = material.HeightFactor,
		.IsDepthMap = material.IsDepthMap,
		.RoughnessFactor = material.RoughnessFactor,
		.MetallicFactor = material.MetallicFactor,
		.AmbientOccFactor = material.AmbientOccFactor
	};
}

struct RendererData
{
	static constexpr uint32_t MaxQuads	   = 5000;
	static constexpr uint32_t MaxVertices  = MaxQuads * 4;
	static constexpr uint32_t MaxIndices   = MaxQuads * 6;
	static constexpr uint32_t MaxInstances = MaxQuads / 4;
	static constexpr uint32_t MaxInstancesOfType = MaxInstances / 5;

	RendererStats Stats;
	Clock RenderClock;

	uint32_t InstancesCount = 0;
	std::unordered_map<int32_t, MeshBufferData> MeshesData;

	std::shared_ptr<VertexArray>  ScreenQuadVertexArray;
	std::shared_ptr<VertexBuffer> ScreenQuadVertexBuffer;
	std::shared_ptr<Shader>		  ScreenQuadShader;

	std::shared_ptr<VertexArray>  EnvMapVertexArray;
	std::shared_ptr<VertexBuffer> EnvMapVertexBuffer;
	std::shared_ptr<Shader>		  EnvMapShader;
	std::shared_ptr<Shader>		  PrefilterShader;
	std::shared_ptr<Shader>		  BRDF_Shader;
	std::shared_ptr<Shader>		  SkyboxShader;

	std::shared_ptr<VertexArray>  LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;
	std::shared_ptr<Shader>		  LineShader;
	uint32_t	LineVertexCount = 0;
	LineVertex* LineBufferBase  = nullptr;
	LineVertex* LineBufferPtr   = nullptr;

	std::shared_ptr<Shader>	DefaultShader;
	std::shared_ptr<Shader> CurrentShader;

	std::shared_ptr<UniformBuffer> MatricesBuffer;
	std::shared_ptr<UniformBuffer> CameraBuffer;
	std::shared_ptr<UniformBuffer> LightsBuffer;
	std::shared_ptr<UniformBuffer> MaterialsBuffer;

	std::vector<DirLightBufferData>	  DirLightsData;
	std::vector<PointLightBufferData> PointLightsData;
	std::vector<SpotLightBufferData>  SpotLightsData;
	std::vector<MaterialsBufferData>  MaterialsData;

	uint32_t BoundTexturesCount = 1;
	std::array<int32_t, 64> TextureBindings;
};

static RendererData s_Data{};

static void PrintDriversInfo()
{
	GLCall(LOG_INFO("GPU Vendor:\t{}",     (char*)glGetString(GL_VENDOR)));
	GLCall(LOG_INFO("GPU:\t\t{}",		   (char*)glGetString(GL_RENDERER)));
	GLCall(LOG_INFO("OpenGL version:\t{}", (char*)glGetString(GL_VERSION)));

	int32_t data{};

	GLCall(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &data));
	LOG_INFO("Texture units:\t{}", data);

	GLCall(glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &data));
	LOG_INFO("Max SSBO size:\t{} bytes", data);

	GLCall(glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &data));
	LOG_INFO("SSBO bindings:\t{}", data);

	GLCall(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &data));
	LOG_INFO("UBO bindings:\t{}", data);

	GLCall(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &data));
	LOG_INFO("Max UBO size:\t{} bytes", data);

	GLCall(glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &data));
	LOG_INFO("Max uniform locations:\t{}", data);

	GLCall(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &data));
	LOG_INFO("Max uniform array size:\t{}", data);
}

static Mesh GenerateMeshData(VertexData vertexData)
{
	Mesh mesh{};
	auto& [vertices, indices] = vertexData;
	mesh.VAO = std::make_shared<VertexArray>();
	mesh.VBO = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex), vertices.size());

	VertexBufferLayout layout;
	layout.Push<float>(3); // 0 Position
	layout.Push<float>(3); // 1 Normal
	layout.Push<float>(3); // 2 Tangent
	layout.Push<float>(3); // 3 Bitangent
	layout.Push<float>(2); // 4 Texture UV
	
	std::unique_ptr<IndexBuffer> ibo = std::make_unique<IndexBuffer>(indices.data(), (uint32_t)indices.size());
	mesh.VAO->AddBuffers(mesh.VBO, ibo, layout);

	layout.Clear();
	layout.Push<float>(4); // 5  Transform
	layout.Push<float>(4); // 6  Transform
	layout.Push<float>(4); // 7  Transform
	layout.Push<float>(4); // 8  Transform
	layout.Push<float>(1); // 9  Material slot
	layout.Push<float>(1); // 10 Entity ID
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
	GLCall(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

	PrintDriversInfo();

	{
		SCOPE_PROFILE("Quad mesh init");

		Mesh quadMesh = GenerateMeshData(QuadMeshData());
		quadMesh.Name = "Quad";

		int32_t meshID = AssetManager::AddMesh(quadMesh, AssetManager::MESH_PLANE);
		s_Data.MeshesData[meshID].Instances.reserve(s_Data.MaxInstancesOfType);
	}

	{
		SCOPE_PROFILE("Cube mesh init");

		Mesh cubeMesh = GenerateMeshData(CubeMeshData());
		cubeMesh.Name = "Cube";

		int32_t meshID = AssetManager::AddMesh(cubeMesh, AssetManager::MESH_CUBE);
		s_Data.MeshesData[meshID].Instances.reserve(s_Data.MaxInstancesOfType);
	}

	{
		SCOPE_PROFILE("Sphere mesh init");

		Mesh sphereMesh = GenerateMeshData(SphereMeshData());
		sphereMesh.Name = "Sphere";

		int32_t meshID = AssetManager::AddMesh(sphereMesh, AssetManager::MESH_SPHERE);
		s_Data.MeshesData[meshID].Instances.reserve(s_Data.MaxInstancesOfType);
	}

	{
		SCOPE_PROFILE("Shaders init + default textures");

		s_Data.DefaultShader = std::make_shared<Shader>("resources/shaders/Default.vert", "resources/shaders/Default.frag");
		s_Data.DefaultShader->Bind();
		for (int32_t i = 0; i < s_Data.TextureBindings.size(); i++)
		{
			s_Data.DefaultShader->SetUniform1i("u_Textures[" + std::to_string(i) + "]", i);
		}

		uint8_t whitePixel[] = { 255, 255, 255, 255 };
		std::shared_ptr<Texture> defaultAlbedo = std::make_shared<Texture>(whitePixel, 1, 1, "Default white");
		AssetManager::AddTexture(defaultAlbedo, AssetManager::TEXTURE_WHITE);

		uint8_t normalPixel[] = { 127, 127, 255, 255 };
		std::shared_ptr<Texture> defaultNormal = std::make_shared<Texture>(normalPixel, 1, 1, "Default normal");
		AssetManager::AddTexture(defaultNormal, AssetManager::TEXTURE_NORMAL);

		uint8_t blackPixel[] = { 0, 0, 0, 255 };
		std::shared_ptr<Texture> defaultBlack = std::make_shared<Texture>(blackPixel, 1, 1, "Default black");
		AssetManager::AddTexture(defaultBlack, AssetManager::TEXTURE_BLACK);

		Material mat{};
		mat.Name = "Default material";
		mat.AlbedoTextureID = AssetManager::TEXTURE_WHITE;
		mat.NormalTextureID = AssetManager::TEXTURE_NORMAL;
		mat.HeightTextureID = AssetManager::TEXTURE_BLACK;
		AssetManager::AddMaterial(mat, AssetManager::MATERIAL_DEFAULT);

		s_Data.CurrentShader = s_Data.DefaultShader;
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
		SCOPE_PROFILE("Environment map init");

		std::vector<float> vertices = SkyboxMeshData();
		s_Data.EnvMapVertexArray = std::make_shared<VertexArray>();
		s_Data.EnvMapVertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(float));

		VertexBufferLayout layout;
		layout.Push<float>(3); // Position

		s_Data.EnvMapVertexArray->AddVertexBuffer(s_Data.EnvMapVertexBuffer, layout);
		s_Data.EnvMapShader = std::make_shared<Shader>("resources/shaders/EnvMapper.vert", "resources/shaders/EnvMapper.frag");
		s_Data.EnvMapShader->Bind();
		s_Data.EnvMapShader->SetUniform1i("u_EquirectangularEnvMap", 0);

		s_Data.PrefilterShader = std::make_shared<Shader>("resources/shaders/EnvMapper.vert", "resources/shaders/EnvPrefilter.frag");
		s_Data.PrefilterShader->Bind();
		s_Data.PrefilterShader->SetUniform1i("u_EnvironmentMap", 0);

		s_Data.BRDF_Shader = std::make_shared<Shader>("resources/shaders/ScreenQuad.vert", "resources/shaders/BRDF.frag");
		s_Data.BRDF_Shader->Bind();

		s_Data.SkyboxShader = std::make_shared<Shader>("resources/shaders/Skybox.vert", "resources/shaders/Skybox.frag");
		s_Data.SkyboxShader->Bind();
		s_Data.SkyboxShader->SetUniform1i("u_Cubemap", 0);
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

		s_Data.CameraBuffer = std::make_shared<UniformBuffer>(nullptr, sizeof(glm::vec4) + 2 * sizeof(float));
		s_Data.CameraBuffer->BindBufferRange(1, 0, sizeof(glm::vec4) + 2 * sizeof(float));

		s_Data.LightsBuffer = std::make_shared<UniformBuffer>(nullptr,
			3 * sizeof(int32_t) + 128 * (sizeof(DirLightBufferData) + sizeof(PointLightBufferData) + sizeof(SpotLightBufferData)));
		s_Data.LightsBuffer->BindBufferRange(2, 0,
			3 * sizeof(int32_t) + 128 * (sizeof(DirLightBufferData) + sizeof(PointLightBufferData) + sizeof(SpotLightBufferData)));

		s_Data.MaterialsBuffer = std::make_shared<UniformBuffer>(nullptr, 128 * sizeof(MaterialsBufferData));
		s_Data.MaterialsBuffer->BindBufferRange(3, 0, 128 * sizeof(MaterialsBufferData));
	}

	memset(&s_Data.Stats, 0, sizeof(RendererStats));
}

void Renderer::Shutdown()
{
	delete[] s_Data.LineBufferBase;

	s_Data.ScreenQuadVertexArray = nullptr;
	s_Data.ScreenQuadVertexBuffer = nullptr;
	s_Data.ScreenQuadShader = nullptr;

	s_Data.LineVertexArray = nullptr;
	s_Data.LineVertexBuffer = nullptr;
	s_Data.LineShader = nullptr;

	s_Data.EnvMapVertexArray = nullptr;
	s_Data.EnvMapVertexBuffer = nullptr;
	s_Data.EnvMapShader = nullptr;
	s_Data.PrefilterShader = nullptr;
	s_Data.BRDF_Shader = nullptr;
	s_Data.SkyboxShader = nullptr;

	s_Data.DefaultShader = nullptr;
	s_Data.CurrentShader = nullptr;

	s_Data.MatricesBuffer = nullptr;
	s_Data.CameraBuffer = nullptr;
	s_Data.LightsBuffer = nullptr;
	s_Data.MaterialsBuffer = nullptr;
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

	s_Data.CameraBuffer->Bind();
	s_Data.CameraBuffer->SetData(glm::value_ptr(camera.Position), sizeof(glm::vec3));
	s_Data.CameraBuffer->SetData(&camera.Exposure, sizeof(float), sizeof(glm::vec4));
	s_Data.CameraBuffer->SetData(&camera.Gamma, sizeof(float), sizeof(glm::vec4) + sizeof(float));

	StartBatch();
}

void Renderer::SceneEnd()
{
	Flush();
}

void Renderer::Flush()
{
	s_Data.LightsBuffer->SetData(s_Data.DirLightsData.data(), s_Data.DirLightsData.size() * sizeof(DirLightBufferData));
	uint32_t offset = 128 * sizeof(DirLightBufferData);

	s_Data.LightsBuffer->SetData(s_Data.PointLightsData.data(), s_Data.PointLightsData.size() * sizeof(PointLightBufferData), offset);
	offset += 128 * sizeof(PointLightBufferData);

	s_Data.LightsBuffer->SetData(s_Data.SpotLightsData.data(), s_Data.SpotLightsData.size() * sizeof(SpotLightBufferData), offset);
	offset += 128 * sizeof(SpotLightBufferData);

	int32_t count = s_Data.DirLightsData.size();
	s_Data.LightsBuffer->SetData(&count, sizeof(int32_t), offset);
	offset += sizeof(int32_t);

	count = s_Data.PointLightsData.size();
	s_Data.LightsBuffer->SetData(&count, sizeof(int32_t), offset);
	offset += sizeof(int32_t);

	count = s_Data.SpotLightsData.size();
	s_Data.LightsBuffer->SetData(&count, sizeof(int32_t), offset);

	s_Data.MaterialsBuffer->SetData(s_Data.MaterialsData.data(), s_Data.MaterialsData.size() * sizeof(MaterialsBufferData));

	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		// Offset by 1 to account for BRDF map
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

void Renderer::ResetStats()
{
	s_Data.RenderClock.Restart();
	memset(&s_Data.Stats, 0, sizeof(RendererStats));
}

RendererStats Renderer::Stats()
{
	s_Data.Stats.RenderTimeInMS = s_Data.RenderClock.GetElapsedTime();
	return s_Data.Stats;
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

	Material material;
	material.Color = color;

	SubmitMesh(transform, mesh, material, 0);
}

void Renderer::SubmitMesh(const glm::mat4& transform, const MeshComponent& mesh, const Material& material, int32_t entityID)
{
	if (s_Data.MeshesData[mesh.MeshID].CurrentInstancesCount >= s_Data.MaxInstancesOfType
		|| s_Data.InstancesCount >= s_Data.MaxInstances
		|| s_Data.BoundTexturesCount >= s_Data.TextureBindings.size() - 1
		|| s_Data.MaterialsData.size() >= 127)
	{
		NextBatch();
	}

	std::vector<MeshInstance>& instances = s_Data.MeshesData[mesh.MeshID].Instances;
	MeshInstance& instance = instances.emplace_back();
	instance.Transform = transform;
	instance.EntityID = (float)entityID + 1.0f;
	
	s_Data.MeshesData[mesh.MeshID].CurrentInstancesCount++;
	s_Data.InstancesCount++;

	std::shared_ptr<Texture> albedo	= AssetManager::GetTexture(material.AlbedoTextureID);
	int32_t albedoIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == albedo->GetID())
		{
			albedoIdx = i;
		}
	}
	if(albedoIdx == -1)
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = albedo->GetID();
		albedoIdx = s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	std::shared_ptr<Texture> normal = AssetManager::GetTexture(material.NormalTextureID);
	int32_t normalIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == normal->GetID())
		{
			normalIdx = i;
		}
	}
	if (normalIdx == -1)
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = normal->GetID();
		normalIdx = s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	std::shared_ptr<Texture> height = AssetManager::GetTexture(material.HeightTextureID);
	int32_t heightIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == height->GetID())
		{
			heightIdx = i;
		}
	}
	if (heightIdx == -1)
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = height->GetID();
		heightIdx = s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	std::shared_ptr<Texture> roughness = AssetManager::GetTexture(material.RoughnessTextureID);
	int32_t roughnessIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == roughness->GetID())
		{
			roughnessIdx = i;
		}
	}
	if (roughnessIdx == -1)
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = roughness->GetID();
		roughnessIdx = s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	std::shared_ptr<Texture> metallic = AssetManager::GetTexture(material.MetallicTextureID);
	int32_t metallicIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == metallic->GetID())
		{
			metallicIdx = i;
		}
	}
	if (metallicIdx == -1)
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = metallic->GetID();
		metallicIdx = s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	std::shared_ptr<Texture> ao = AssetManager::GetTexture(material.AmbientOccTextureID);
	int32_t aoIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == ao->GetID())
		{
			aoIdx = i;
		}
	}
	if (aoIdx == -1)
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = ao->GetID();
		aoIdx = s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	int32_t materialIdx = -1;
	for (int32_t i = 0; i < s_Data.MaterialsData.size(); i++)
	{
		MaterialsBufferData& matData = s_Data.MaterialsData[i];
		bool sameTextures = matData.AlbedoTextureSlot == albedoIdx && matData.NormalTextureSlot == normalIdx
			&& matData.HeightTextureSlot == heightIdx && matData.RoughnessTextureSlot == roughnessIdx
			&& matData.MetallicTextureSlot == metallicIdx && matData.AmbientOccTextureSlot == aoIdx;

		if (MaterialToBufferCmp(material, matData))
		{
			materialIdx = i;
			break;
		}
	}

	if (materialIdx == -1)
	{
		instance.MaterialSlot = (float)s_Data.MaterialsData.size();
		s_Data.MaterialsData.push_back(MaterialToBuffer(material));
		s_Data.MaterialsData.back().AlbedoTextureSlot	  = albedoIdx;
		s_Data.MaterialsData.back().NormalTextureSlot	  = normalIdx;
		s_Data.MaterialsData.back().HeightTextureSlot	  = heightIdx;
		s_Data.MaterialsData.back().RoughnessTextureSlot  = roughnessIdx;
		s_Data.MaterialsData.back().MetallicTextureSlot   = metallicIdx;
		s_Data.MaterialsData.back().AmbientOccTextureSlot = aoIdx;
	}
	else
	{
		instance.MaterialSlot = (float)materialIdx;
	}

	s_Data.Stats.ObjectsRendered++;
}

void Renderer::DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawElements(primitiveType, vao->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr));

	s_Data.Stats.DrawCalls++;
}

void Renderer::DrawIndexedInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawElementsInstanced(primitiveType, vao->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr, instances));

	s_Data.Stats.DrawCalls++;
}

void Renderer::DrawArrays(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t vertexCount, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawArrays(primitiveType, 0, vertexCount));

	s_Data.Stats.DrawCalls++;
}

void Renderer::DrawArraysInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawArraysInstanced(primitiveType, 0, vao->VertexCount(), instances));

	s_Data.Stats.DrawCalls++;
}

void Renderer::DrawScreenQuad()
{
	GLCall(glDisable(GL_DEPTH_TEST));
	DrawArrays(s_Data.ScreenQuadShader, s_Data.ScreenQuadVertexArray, 6);
	GLCall(glEnable(GL_DEPTH_TEST));
}

std::shared_ptr<CubemapFramebuffer> Renderer::CreateEnvCubemap(std::shared_ptr<Texture> hdrEnvMap, const glm::uvec2& faceSize)
{
	std::shared_ptr<CubemapFramebuffer> cfb = std::make_shared<CubemapFramebuffer>(faceSize);
	glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	s_Data.MatricesBuffer->Bind();
	s_Data.MatricesBuffer->SetData(glm::value_ptr(captureProj), sizeof(glm::mat4));

	cfb->Bind();
	hdrEnvMap->Bind();
	s_Data.EnvMapShader->Bind();
	s_Data.EnvMapShader->SetUniform1i("u_EquirectangularEnvMap", 0);

	GLenum irradianceBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	GLCall(glDrawBuffers(2, irradianceBuffers));
	for (uint32_t i = 0; i < 6; i++)
	{
		s_Data.MatricesBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
		cfb->SetCubemapFaceTarget(i);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawArrays(s_Data.EnvMapShader, s_Data.EnvMapVertexArray, 36);
	}
	cfb->BindCubemap();
	GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));

	constexpr uint32_t MIPMAP_LEVELS = 5;
	s_Data.PrefilterShader->Bind();
	for (uint32_t mip = 0; mip < MIPMAP_LEVELS; mip++)
	{
		uint32_t mipW = 128 / (mip + 1);
		uint32_t mipH = 128 / (mip + 1);
		cfb->ResizeRenderbuffer({ mipW, mipH });

		float roughness = (float)mip / (float)(MIPMAP_LEVELS - 1);
		s_Data.PrefilterShader->SetUniform1f("u_Roughness", roughness);

		for (uint32_t i = 0; i < 6; i++)
		{
			s_Data.MatricesBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
			cfb->SetPrefilterFaceTarget(i, mip);
			Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			DrawArrays(s_Data.PrefilterShader, s_Data.EnvMapVertexArray, 36);
		}
	}

	cfb->ResizeRenderbuffer(faceSize);
	cfb->SetBRDF_Target();
	Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawArrays(s_Data.BRDF_Shader, s_Data.ScreenQuadVertexArray, 6);
	cfb->UnbindMaps();
	cfb->Unbind();

	return cfb;
}

void Renderer::DrawSkybox(std::shared_ptr<CubemapFramebuffer> cfb)
{
	cfb->BindCubemap();
	GLCall(glDepthFunc(GL_LEQUAL));
	DrawArrays(s_Data.SkyboxShader, s_Data.EnvMapVertexArray, 36);
	GLCall(glDepthFunc(GL_LESS));

	cfb->BindIrradianceMap(50);
	cfb->BindPrefilterMap(51);
	cfb->BindBRDF_Map(52);
	s_Data.CurrentShader->Bind();
	s_Data.CurrentShader->SetUniform1i("u_IrradianceMap", 50);
	s_Data.CurrentShader->SetUniform1i("u_PrefilterMap", 51);
	s_Data.CurrentShader->SetUniform1i("u_BRDF_LUT", 52);
}

void Renderer::AddDirectionalLight(const TransformComponent& transform, const DirectionalLightComponent& light)
{
	glm::vec3 dir = glm::toMat3(glm::quat(transform.Rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
	s_Data.DirLightsData.push_back({ glm::vec4(dir, 1.0f), glm::vec4(light.Color * light.Intensity, 1.0f) });
}

void Renderer::AddPointLight(const glm::vec3& position, const PointLightComponent& light)
{
	s_Data.PointLightsData.push_back({ glm::vec4(position, light.LinearTerm), glm::vec4(light.Color * light.Intensity, light.QuadraticTerm) });
}

void Renderer::AddSpotLight(const TransformComponent& transform, const SpotLightComponent& light)
{
	glm::vec3 dir = glm::toMat3(glm::quat(transform.Rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
	s_Data.SpotLightsData.push_back({
		glm::vec4(transform.Position, glm::cos(glm::radians(light.Cutoff))),
		glm::vec4(dir, glm::cos(glm::radians(light.Cutoff - light.EdgeSmoothness))),
		glm::vec4(light.Color * light.Intensity, light.LinearTerm),
		light.QuadraticTerm
	});
}

void Renderer::SetBlur(bool enabled)
{
	s_Data.ScreenQuadShader->Bind();
	s_Data.ScreenQuadShader->SetUniform1i("u_BlurEnabled", enabled ? 1 : 0);
}

void Renderer::SetLight(bool enabled)
{
	s_Data.CurrentShader->Bind();
	s_Data.CurrentShader->SetUniformBool("u_IsLightSource", enabled);
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

void Renderer::SetWireframe(bool enabled)
{
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL));
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

	s_Data.DirLightsData.clear();
	s_Data.PointLightsData.clear();
	s_Data.SpotLightsData.clear();
	s_Data.MaterialsData.clear();

	s_Data.BoundTexturesCount = 0;
}

void Renderer::NextBatch()
{
	Flush();

	s_Data.InstancesCount = 0;
	for (auto& [meshID, data] : s_Data.MeshesData)
	{
		data.CurrentInstancesCount = 0;
		data.Instances.clear();
		data.Instances.reserve(s_Data.MaxInstancesOfType);
	}

	s_Data.MaterialsData.clear();
	s_Data.LineVertexCount = 0;
	s_Data.LineBufferPtr = s_Data.LineBufferBase;
	s_Data.BoundTexturesCount = 0;
}
