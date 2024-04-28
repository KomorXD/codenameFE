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
		&& lhs.HeightFactor == rhs.HeightFactor
		&& lhs.IsDepthMap == rhs.IsDepthMap
		&& lhs.RoughnessFactor == rhs.RoughnessFactor
		&& lhs.MetallicFactor == rhs.MetallicFactor
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

	uint32_t InstancesCount = 0;
	std::unordered_map<int32_t, MeshBufferData> MeshesData;

	std::shared_ptr<VertexArray>  ScreenQuadVertexArray;
	std::shared_ptr<VertexBuffer> ScreenQuadVertexBuffer;
	std::shared_ptr<Shader>		  ScreenQuadShader;

	std::shared_ptr<VertexArray>  EnvMapVertexArray;
	std::shared_ptr<VertexBuffer> EnvMapVertexBuffer;
	std::shared_ptr<Shader>		  EnvMapShader;
	std::shared_ptr<Shader>		  IrradianceShader;
	std::shared_ptr<Shader>		  PrefilterShader;
	std::shared_ptr<Shader>		  SkyboxShader;
	std::shared_ptr<Texture>	  BRDF_Map;

	std::shared_ptr<Framebuffer> BloomFBO;
	std::shared_ptr<Shader> BloomDownsamplerShader;
	std::shared_ptr<Shader> BloomUpsamplerShader;

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

	uint32_t BoundTexturesCount = 0;
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
		s_Data.ScreenQuadShader->SetUniform1i("u_BloomTexture", 1);
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

		s_Data.IrradianceShader = std::make_shared<Shader>("resources/shaders/EnvMapper.vert", "resources/shaders/Irradiance.frag");
		s_Data.IrradianceShader->Bind();
		s_Data.IrradianceShader->SetUniform1i("u_EnvMap", 0);

		s_Data.PrefilterShader = std::make_shared<Shader>("resources/shaders/EnvMapper.vert", "resources/shaders/EnvPrefilter.frag");
		s_Data.PrefilterShader->Bind();
		s_Data.PrefilterShader->SetUniform1i("u_EnvironmentMap", 0);

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

	{
		SCOPE_PROFILE("Bloom setup");

		glm::ivec2 mipSize(1024);
		s_Data.BloomFBO = std::make_shared<Framebuffer>(1);
		for (uint32_t i = 0; i < 5; i++)
		{
			mipSize /= 2;
			s_Data.BloomFBO->AddColorAttachment({
				.Type = ColorAttachmentType::TEX_2D,
				.Format = TextureFormat::R11_G11_B10,
				.Wrap = GL_CLAMP_TO_EDGE,
				.MinFilter = GL_LINEAR,
				.MagFilter = GL_LINEAR,
				.Size = mipSize,
				.GenMipmaps = false
			});
		}
		s_Data.BloomFBO->DrawToColorAttachment(0, 0);
		assert(s_Data.BloomFBO->IsComplete() && "Incomplete framebuffer!");
		s_Data.BloomFBO->Unbind();

		s_Data.BloomDownsamplerShader = std::make_shared<Shader>("resources/shaders/ScreenQuad.vert", "resources/shaders/BloomDownsampler.frag");
		s_Data.BloomDownsamplerShader->Bind();
		s_Data.BloomDownsamplerShader->SetUniform1i("u_SourceTexture", 0);

		s_Data.BloomUpsamplerShader = std::make_shared<Shader>("resources/shaders/ScreenQuad.vert", "resources/shaders/BloomUpsampler.frag");
		s_Data.BloomUpsamplerShader->Bind();
		s_Data.BloomUpsamplerShader->SetUniform1i("u_SourceTexture", 0);
	}

	{
		SCOPE_PROFILE("Creating BRDF map");
		
		GLuint fbo{};
		GLuint rbo{};
		GLCall(glGenFramebuffers(1, &fbo));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
		GLCall(glGenRenderbuffers(1, &rbo));
		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512));
		GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo));

		GLuint brdfTex{};
		GLCall(glGenTextures(1, &brdfTex));
		GLCall(glBindTexture(GL_TEXTURE_2D, brdfTex));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfTex, 0));
		GLCall(glViewport(0, 0, 512, 512)); 
		
		std::shared_ptr<Shader> brdfShader = std::make_shared<Shader>("resources/shaders/ScreenQuad.vert", "resources/shaders/BRDF.frag");
		brdfShader->Bind();
		Clear();
		DrawArrays(brdfShader, s_Data.ScreenQuadVertexArray, 6);
		s_Data.BRDF_Map = std::make_shared<Texture>((uint32_t)brdfTex, "BRDF Map", TextureFormat::RG16F);

		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		GLCall(glDeleteRenderbuffers(1, &rbo));
		GLCall(glDeleteFramebuffers(1, &fbo));
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
	s_Data.IrradianceShader = nullptr;
	s_Data.PrefilterShader = nullptr;
	s_Data.SkyboxShader = nullptr;
	s_Data.BRDF_Map = nullptr;

	s_Data.BloomFBO = nullptr;
	s_Data.BloomDownsamplerShader = nullptr;
	s_Data.BloomUpsamplerShader = nullptr;

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
	memset(&s_Data.Stats, 0, sizeof(RendererStats));
}

RendererStats Renderer::Stats()
{
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
		|| s_Data.InstancesCount >= s_Data.MaxInstances)
	{
		NextBatch();
	}

	std::shared_ptr<Texture> textures[] = {
		AssetManager::GetTexture(material.AlbedoTextureID),
		AssetManager::GetTexture(material.NormalTextureID),
		AssetManager::GetTexture(material.HeightTextureID),
		AssetManager::GetTexture(material.RoughnessTextureID),
		AssetManager::GetTexture(material.MetallicTextureID),
		AssetManager::GetTexture(material.AmbientOccTextureID)
	};
	int32_t textureIdxs[] = { -1, -1, -1, -1, -1, -1 };
	size_t newTextures = 0;

	// Check for a duplicate in existing texture bindings
	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < (size_t)s_Data.BoundTexturesCount; j++)
		{
			if (s_Data.TextureBindings[j] == textures[i]->GetID())
			{
				textureIdxs[i] = j;
				break;
			}
		}

		if (textureIdxs[i] == -1)
		{
			newTextures++;
		}
	}
	if (s_Data.BoundTexturesCount + newTextures >= s_Data.TextureBindings.size())
	{
		NextBatch();
	}

	// Check for duplicates in this material's textures
	newTextures = 0;
	for (size_t i = 0; i < 6; i++)
	{
		if (textureIdxs[i] != -1)
		{
			continue;
		}

		bool doAddNew = true;
		for (uint32_t j = s_Data.BoundTexturesCount - newTextures; j < s_Data.BoundTexturesCount; j++)
		{
			if (s_Data.TextureBindings[j] == textures[i]->GetID())
			{
				textureIdxs[i] = j;
				doAddNew = false;
			}
		}

		if (doAddNew)
		{
			s_Data.TextureBindings[s_Data.BoundTexturesCount] = textures[i]->GetID();
			textureIdxs[i] = s_Data.BoundTexturesCount++;
			newTextures++;
		}
	}

	// Check for a duplicate material in their buffer
	int32_t materialIdx = -1;
	for (int32_t i = 0; i < s_Data.MaterialsData.size(); i++)
	{
		MaterialsBufferData& mbd = s_Data.MaterialsData[i];
		bool areTexturesTheSame = 
			mbd.AlbedoTextureSlot	  == textureIdxs[0] &&
			mbd.NormalTextureSlot	  == textureIdxs[1] &&
			mbd.HeightTextureSlot	  == textureIdxs[2] &&
			mbd.RoughnessTextureSlot  == textureIdxs[3] &&
			mbd.MetallicTextureSlot	  == textureIdxs[4] &&
			mbd.AmbientOccTextureSlot == textureIdxs[5];

		if (MaterialToBufferCmp(material, mbd) && areTexturesTheSame)
		{
			materialIdx = i;
			break;
		}
	}

	if (materialIdx == -1)
	{
		if (s_Data.MaterialsData.size() >= 127)
		{
			NextBatch();
		}

		materialIdx = s_Data.MaterialsData.size();
		s_Data.MaterialsData.push_back(MaterialToBuffer(material));
		s_Data.MaterialsData.back().AlbedoTextureSlot = textureIdxs[0];
		s_Data.MaterialsData.back().NormalTextureSlot = textureIdxs[1];
		s_Data.MaterialsData.back().HeightTextureSlot = textureIdxs[2];
		s_Data.MaterialsData.back().RoughnessTextureSlot = textureIdxs[3];
		s_Data.MaterialsData.back().MetallicTextureSlot = textureIdxs[4];
		s_Data.MaterialsData.back().AmbientOccTextureSlot = textureIdxs[5];
	}

	std::vector<MeshInstance>& instances = s_Data.MeshesData[mesh.MeshID].Instances;
	MeshInstance& instance = instances.emplace_back();
	instance.Transform = transform;
	instance.EntityID = (float)entityID + 1.0f;
	instance.MaterialSlot = (float)materialIdx;
	s_Data.MeshesData[mesh.MeshID].CurrentInstancesCount++;
	s_Data.InstancesCount++;
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

void Renderer::Bloom(const std::unique_ptr<Framebuffer>& hdrFBO)
{
	const std::vector<ColorAttachment>& mips = s_Data.BloomFBO->ColorAttachments();
	s_Data.BloomFBO->Bind();

	// Bloom downsampling
	glm::ivec2 viewportSize = hdrFBO->ColorAttachmentSize(0);
	s_Data.BloomDownsamplerShader->Bind();
	s_Data.BloomDownsamplerShader->SetUniform2f("u_SourceResolution", viewportSize);
	s_Data.BloomDownsamplerShader->SetUniformBool("u_FirstMip", true);
	hdrFBO->BindColorAttachment(0);
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));
	GLCall(glDisable(GL_DEPTH_TEST));

	for (size_t i = 0; i < mips.size(); i++)
	{
		const ColorAttachment& mip = mips[i];
		GLCall(glViewport(0, 0, mip.spec.Size.x, mip.spec.Size.y));
		s_Data.BloomFBO->DrawToColorAttachment(i, 0);

		DrawArrays(s_Data.BloomDownsamplerShader, s_Data.ScreenQuadVertexArray, 6);

		s_Data.BloomDownsamplerShader->SetUniform2f("u_SourceResolution", mip.spec.Size);
		s_Data.BloomDownsamplerShader->SetUniformBool("u_FirstMip", false);
		s_Data.BloomFBO->BindColorAttachment(i);
	}

	// Bloom upsampling
	s_Data.BloomUpsamplerShader->Bind();
	s_Data.BloomUpsamplerShader->SetUniform1f("u_FilterRadius", 0.005f);
	GLCall(glBlendFunc(GL_ONE, GL_ONE));
	GLCall(glBlendEquation(GL_FUNC_ADD));

	for (size_t i = mips.size() - 1; i > 0; i--)
	{
		const ColorAttachment& mip = mips[i];
		const ColorAttachment& nextMip = mips[i - 1];

		s_Data.BloomFBO->BindColorAttachment(i);
		GLCall(glViewport(0, 0, nextMip.spec.Size.x, nextMip.spec.Size.y));
		s_Data.BloomFBO->DrawToColorAttachment(i - 1, 0);

		DrawArrays(s_Data.BloomUpsamplerShader, s_Data.ScreenQuadVertexArray, 6);
	}
	GLCall(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glViewport(0, 0, viewportSize.x, viewportSize.y));
	s_Data.BloomFBO->BindColorAttachment(0, 1);
}

void Renderer::SetBloomStrength(float strength)
{
	s_Data.ScreenQuadShader->Bind();
	s_Data.ScreenQuadShader->SetUniform1f("u_BloomStrength", strength);
}

std::shared_ptr<Framebuffer> Renderer::CreateEnvCubemap(std::shared_ptr<Texture> hdrEnvMap, const glm::uvec2& faceSize)
{
	std::shared_ptr<Framebuffer> cfb = std::make_shared<Framebuffer>(1);
	cfb->AddRenderbuffer({
		.Type = RenderbufferType::DEPTH,
		.Size = faceSize
	});
	cfb->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_CUBEMAP,
		.Format = TextureFormat::RGB16F,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR_MIPMAP_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = faceSize,
		.GenMipmaps = true
	});

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
	cfb->BindRenderbuffer();
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));

	hdrEnvMap->Bind();
	s_Data.EnvMapShader->Bind();
	for (uint32_t i = 0; i < 6; i++)
	{
		s_Data.MatricesBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
		cfb->DrawToCubeColorAttachment(0, 0, i);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawArrays(s_Data.EnvMapShader, s_Data.EnvMapVertexArray, 36);
	}
	cfb->BindColorAttachment(0);
	GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

	cfb->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_CUBEMAP,
		.Format = TextureFormat::RGB16F,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = faceSize,
		.GenMipmaps = false
	});
	cfb->BindColorAttachment(0);
	s_Data.IrradianceShader->Bind();
	for (uint32_t i = 0; i < 6; i++)
	{
		s_Data.MatricesBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
		cfb->DrawToCubeColorAttachment(1, 0, i);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawArrays(s_Data.IrradianceShader, s_Data.EnvMapVertexArray, 36);
	}

	cfb->AddColorAttachment({
		.Type = ColorAttachmentType::TEX_CUBEMAP,
		.Format = TextureFormat::RGB16F,
		.Wrap = GL_CLAMP_TO_EDGE,
		.MinFilter = GL_LINEAR_MIPMAP_LINEAR,
		.MagFilter = GL_LINEAR,
		.Size = { 128, 128 },
		.GenMipmaps = true
	});
	GLCall(glViewport(0, 0, 128, 128));

	constexpr uint32_t MIPMAP_LEVELS = 5;
	cfb->BindColorAttachment(0);
	s_Data.PrefilterShader->Bind();
	for (uint32_t mip = 0; mip < MIPMAP_LEVELS; mip++)
	{
		uint32_t mipW = 128 / (mip + 1);
		uint32_t mipH = 128 / (mip + 1);
		cfb->ResizeRenderbuffer({ mipW, mipH });
		GLCall(glViewport(0, 0, mipW, mipH));

		float roughness = (float)mip / (float)(MIPMAP_LEVELS - 1);
		s_Data.PrefilterShader->SetUniform1f("u_Roughness", roughness);

		for (uint32_t i = 0; i < 6; i++)
		{
			s_Data.MatricesBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
			cfb->DrawToCubeColorAttachment(2, 0, i, mip);
			Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			DrawArrays(s_Data.PrefilterShader, s_Data.EnvMapVertexArray, 36);
		}
	}
	
	cfb->ResizeRenderbuffer(faceSize);
	cfb->BindRenderbuffer();
	cfb->Unbind();
	return cfb;
}

void Renderer::DrawSkybox(std::shared_ptr<Framebuffer> cfb)
{
	cfb->BindColorAttachment(0);
	GLCall(glDepthFunc(GL_LEQUAL));
	DrawArrays(s_Data.SkyboxShader, s_Data.EnvMapVertexArray, 36);
	GLCall(glDepthFunc(GL_LESS));

	uint32_t irradianceSlot = s_Data.TextureBindings.size();
	uint32_t prefilterSlot = irradianceSlot + 1;
	uint32_t brdfSlot = prefilterSlot + 1;
	cfb->BindColorAttachment(1, irradianceSlot);
	cfb->BindColorAttachment(2, prefilterSlot);
	s_Data.BRDF_Map->Bind(brdfSlot);
	s_Data.CurrentShader->Bind();
	s_Data.CurrentShader->SetUniform1i("u_IrradianceMap", irradianceSlot);
	s_Data.CurrentShader->SetUniform1i("u_PrefilterMap", prefilterSlot);
	s_Data.CurrentShader->SetUniform1i("u_BRDF_LUT", brdfSlot);
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
