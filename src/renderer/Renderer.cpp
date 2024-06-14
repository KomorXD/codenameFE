#include "Renderer.hpp"
#include "../Logger.hpp"
#include "../Timer.hpp"
#include "../Clock.hpp"
#include "Camera.hpp"
#include "PrimitivesGen.hpp"
#include "AssetManager.hpp"
#include "../RandomUtils.hpp"
#include "../Application.hpp"

#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

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
std::shared_ptr<Framebuffer> Renderer::s_TargetFBO;

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
	std::array<glm::mat4, 5> CascadeLightMatrices;
	glm::vec4 Direction;
	glm::vec4 Color;
};

struct PointLightBufferData
{
	std::array<glm::mat4, 6> LightSpaceMatrices;
	std::array<glm::vec4, 6> RenderedDirs;
	glm::vec4 PosAndLinear;
	glm::vec4 ColorAndQuadratic;

	int32_t FacesRendered;
	glm::vec3 Padding;
};

struct SpotlightBufferData
{
	glm::mat4 LightSpaceMatrix;
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

struct CameraBufferData
{
	glm::mat4 Projection;
	glm::mat4 View;
	glm::vec4 Position;
	float Exposure;
	float Gamma;
	float Near;
	float Far;
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
	MaterialsBufferData mbd{};
	mbd.Color = material.Color * (material.Emissive ? material.Emission : 1.0f);
	mbd.Color.a = material.Color.a;
	mbd.TilingFactor = material.TilingFactor;
	mbd.TextureOffset = material.TextureOffset;
	mbd.HeightFactor = material.HeightFactor;
	mbd.IsDepthMap = material.IsDepthMap;
	mbd.RoughnessFactor = material.RoughnessFactor;
	mbd.MetallicFactor = material.MetallicFactor;
	mbd.AmbientOccFactor = material.AmbientOccFactor;

	return mbd;
}

struct GpuSpecs
{
	uint32_t MaxTextureUnits = 64;
};

struct RendererData
{
	static constexpr uint32_t MaxQuads	   = 5000;
	static constexpr uint32_t MaxVertices  = MaxQuads * 4;
	static constexpr uint32_t MaxIndices   = MaxQuads * 6;
	static constexpr uint32_t MaxInstances = MaxQuads / 4;
	static constexpr uint32_t MaxInstancesOfType = MaxInstances / 5;

	static constexpr int32_t CascadesCount = 5;

	int32_t CSM_Slot = -1;
	int32_t PointShadowSlot = -1;
	int32_t SpotlightShadowSlot = -1;

	int32_t IrradianceSlot = -1;
	int32_t PrefilterSlot = -1;
	int32_t BRDF_Slot = -1;

	int32_t OffsetsSlot = -1;

	int32_t MaxMaterials = 32;

	int32_t MaxDirLights = 4;
	int32_t MaxPointLights = 16;
	int32_t MaxSpotlights = 64;

	RendererStats Stats;
	GpuSpecs Specs;

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
	std::shared_ptr<Shader> FlatShader;
	std::shared_ptr<Shader> CurrentShader;

	std::shared_ptr<Framebuffer> ShadowMapsFBO;
	std::shared_ptr<Shader> DirectionalShadowShader;
	std::shared_ptr<Shader> PointShadowShader;
	std::shared_ptr<Shader> SpotlightShadowShader;

	std::shared_ptr<UniformBuffer> CameraBuffer;
	std::shared_ptr<UniformBuffer> MaterialsBuffer;
	std::shared_ptr<UniformBuffer> DirLightsBuffer;
	std::shared_ptr<UniformBuffer> PointLightsBuffer;
	std::shared_ptr<UniformBuffer> SpotlightsBuffer;

	std::vector<MaterialsBufferData>  MaterialsData;

	std::vector<DirLightBufferData>	  DirLightsData;
	std::vector<PointLightBufferData> PointLightsData;
	std::vector<SpotlightBufferData>  SpotlightsData;

	uint32_t OffsetsTexID = 0;
	float OffsetsRadius = 3.0f;

	uint32_t BoundTexturesCount = 0;
	std::vector<int32_t> TextureBindings;

	std::unique_ptr<Framebuffer> G_FBO;
	std::shared_ptr<Shader> G_PassShader;
	std::shared_ptr<Shader> G_LightShader;

	RenderMode RenderMode = RenderMode::FORWARD;
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
	data /= 2;
	s_Data.Specs.MaxTextureUnits = data;
	s_Data.CSM_Slot = data - 7;
	s_Data.PointShadowSlot = data - 6;
	s_Data.SpotlightShadowSlot = data - 5;
	s_Data.IrradianceSlot = data - 4;
	s_Data.PrefilterSlot = data - 3;
	s_Data.BRDF_Slot = data - 2;
	s_Data.OffsetsSlot = data - 1;
	s_Data.TextureBindings.resize((size_t)data - 8);

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

		ShaderSpec spec{};
		spec.Vertex	  = { "resources/shaders/Default.vert", {} };
		spec.Fragment = { 
			"resources/shaders/Default.frag",
			{
				{ "${MATERIALS_COUNT}",		std::to_string(s_Data.MaxMaterials)			 },
				{ "${TEXTURE_UNITS}",		std::to_string(s_Data.Specs.MaxTextureUnits) },
				{ "${MAX_DIR_LIGHTS}",		std::to_string(s_Data.MaxDirLights)			 },
				{ "${MAX_POINT_LIGHTS}",	std::to_string(s_Data.MaxPointLights)		 },
				{ "${MAX_SPOTLIGHTS}",		std::to_string(s_Data.MaxSpotlights)		 },
				{ "${CASCADES_COUNT}",		std::to_string(s_Data.CascadesCount)		 }
			}
		};
		s_Data.DefaultShader = std::make_shared<Shader>(spec);
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

		s_Data.DefaultShader->SetUniform1i("u_DirLightCSM", s_Data.CSM_Slot);
		s_Data.DefaultShader->SetUniform1i("u_PointLightShadowmaps", s_Data.PointShadowSlot);
		s_Data.DefaultShader->SetUniform1i("u_SpotlightShadowmaps", s_Data.SpotlightShadowSlot);
		s_Data.DefaultShader->SetUniform1i("u_OffsetsTexSize", 16);
		s_Data.DefaultShader->SetUniform1i("u_OffsetsFilterSize", 8);
		s_Data.DefaultShader->SetUniform1f("u_OffsetsRadius", s_Data.OffsetsRadius);
		s_Data.DefaultShader->SetUniform1i("u_OffsetsTexture", s_Data.OffsetsSlot);
		s_Data.CurrentShader = s_Data.DefaultShader;

		spec.Vertex = { "resources/shaders/FlatColor.vert", {} };
		spec.Fragment = {
			"resources/shaders/FlatColor.frag",
			{
				{ "${MATERIALS_COUNT}",	std::to_string(s_Data.MaxMaterials)			 },
				{ "${TEXTURE_UNITS}",	std::to_string(s_Data.Specs.MaxTextureUnits) }
			}
		};
		s_Data.FlatShader = std::make_shared<Shader>(spec);
		s_Data.FlatShader->Bind();
		for (int32_t i = 0; i < s_Data.TextureBindings.size(); i++)
		{
			s_Data.FlatShader->SetUniform1i("u_Textures[" + std::to_string(i) + "]", i);
		}
	}

	{
		SCOPE_PROFILE("Shadow mapping setup");

		{
			ShaderSpec spec{};
			spec.Vertex	  = { "resources/shaders/shadows/Directional.vert", {} };
			spec.Fragment = { "resources/shaders/shadows/Directional.frag", {} };
			spec.Geometry = { 
				"resources/shaders/shadows/Directional.geom", 
				{
					{ "${MAX_DIR_LIGHTS}",	std::to_string(s_Data.MaxDirLights)		 },
					{ "${INVOCATIONS}",		std::to_string(s_Data.MaxDirLights)		 },
					{ "${MAX_VERTICES}",	std::to_string(s_Data.CascadesCount * 3) },
					{ "${CASCADES_COUNT}",	std::to_string(s_Data.CascadesCount)	 }
				} 
			};
			s_Data.DirectionalShadowShader = std::make_shared<Shader>(spec);
		}

		{
			ShaderSpec spec{};
			spec.Vertex   = { "resources/shaders/shadows/Point.vert", {} };
			spec.Fragment = { "resources/shaders/shadows/Point.frag", {} };
			spec.Geometry = {
				"resources/shaders/shadows/Point.geom",
				{
					{ "${MAX_POINT_LIGHTS}", std::to_string(s_Data.MaxPointLights) },
					{ "${INVOCATIONS}",		 std::to_string(s_Data.MaxPointLights) }
				}
			};
			s_Data.PointShadowShader = std::make_shared<Shader>(spec);
		}

		{
			ShaderSpec spec{};
			spec.Vertex	  = { "resources/shaders/shadows/Spotlight.vert", {} };
			spec.Fragment = { "resources/shaders/shadows/Spotlight.frag", {} };
			spec.Geometry = {
				"resources/shaders/shadows/Spotlight.geom",
				{
					{ "${MAX_SPOTLIGHTS}",	std::to_string(s_Data.MaxSpotlights)  },
					{ "${INVOCATIONS}",		std::to_string(s_Data.MaxPointLights) }
				}
			};
			s_Data.SpotlightShadowShader = std::make_shared<Shader>(spec);
		}

		s_Data.ShadowMapsFBO = std::make_shared<Framebuffer>(1);

		ColorAttachmentSpec spec{};
		spec.Type = ColorAttachmentType::TEX_2D_ARRAY_SHADOW;
		spec.Format = TextureFormat::DEPTH_32F;
		spec.Wrap = GL_CLAMP_TO_BORDER;
		spec.MinFilter = spec.MagFilter = GL_LINEAR;
		spec.BorderColor = glm::vec4(1.0f);
		spec.Size = { 2048, 2048 };
		spec.Layers = 6;
		spec.GenMipmaps = false;
		s_Data.ShadowMapsFBO->AddColorAttachment(spec);

		spec.Size = { 1024, 1024 };
		spec.Layers = 12;
		s_Data.ShadowMapsFBO->AddColorAttachment(spec);

		spec.Layers = 3;
		s_Data.ShadowMapsFBO->AddColorAttachment(spec);
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

		ShaderSpec spec{};
		spec.Vertex   = { "resources/shaders/ScreenQuad.vert", {} };
		spec.Fragment = { "resources/shaders/ScreenQuad.frag", {} };
		s_Data.ScreenQuadShader = std::make_shared<Shader>(spec);
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

		ShaderSpec spec{};
		spec.Vertex   = { "resources/shaders/env_map/EnvMapper.vert", {} };
		spec.Fragment = { "resources/shaders/env_map/EnvMapper.frag", {} };
		s_Data.EnvMapShader = std::make_shared<Shader>(spec);
		s_Data.EnvMapShader->Bind();
		s_Data.EnvMapShader->SetUniform1i("u_EquirectangularEnvMap", 0);

		spec.Vertex   = { "resources/shaders/env_map/EnvMapper.vert", {} };
		spec.Fragment = { "resources/shaders/env_map/Irradiance.frag", {} };
		s_Data.IrradianceShader = std::make_shared<Shader>(spec);
		s_Data.IrradianceShader->Bind();
		s_Data.IrradianceShader->SetUniform1i("u_EnvMap", 0);

		spec.Vertex   = { "resources/shaders/env_map/EnvMapper.vert", {} };
		spec.Fragment = { "resources/shaders/env_map/EnvPrefilter.frag", {} };
		s_Data.PrefilterShader = std::make_shared<Shader>(spec);
		s_Data.PrefilterShader->Bind();
		s_Data.PrefilterShader->SetUniform1i("u_EnvironmentMap", 0);

		spec.Vertex   = { "resources/shaders/env_map/Skybox.vert", {} };
		spec.Fragment = { "resources/shaders/env_map/Skybox.frag", {} };
		s_Data.SkyboxShader = std::make_shared<Shader>(spec);
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

		ShaderSpec spec{};
		spec.Vertex   = { "resources/shaders/Line.vert", {} };
		spec.Fragment = { "resources/shaders/Line.frag", {} };
		s_Data.LineShader = std::make_shared<Shader>(spec);
	}

	{
		SCOPE_PROFILE("Uniform buffers init");

		s_Data.CameraBuffer = std::make_shared<UniformBuffer>(nullptr, sizeof(CameraBufferData));
		s_Data.CameraBuffer->BindBufferRange(0, 0, sizeof(CameraBufferData));

		s_Data.MaterialsBuffer = std::make_shared<UniformBuffer>(nullptr, 128 * sizeof(MaterialsBufferData));
		s_Data.MaterialsBuffer->BindBufferRange(1, 0, 128 * sizeof(MaterialsBufferData));

		s_Data.DirLightsBuffer = std::make_shared<UniformBuffer>(nullptr, s_Data.MaxDirLights * sizeof(DirLightBufferData) + sizeof(int32_t));
		s_Data.DirLightsBuffer->BindBufferRange(2, 0, s_Data.MaxDirLights * sizeof(DirLightBufferData) + sizeof(int32_t));

		s_Data.PointLightsBuffer = std::make_shared<UniformBuffer>(nullptr, s_Data.MaxPointLights * sizeof(PointLightBufferData) + sizeof(int32_t));
		s_Data.PointLightsBuffer->BindBufferRange(3, 0, s_Data.MaxPointLights * sizeof(PointLightBufferData) + sizeof(int32_t));

		s_Data.SpotlightsBuffer = std::make_shared<UniformBuffer>(nullptr, s_Data.MaxSpotlights * sizeof(SpotlightBufferData) + sizeof(int32_t));
		s_Data.SpotlightsBuffer->BindBufferRange(4, 0, s_Data.MaxSpotlights * sizeof(SpotlightBufferData) + sizeof(int32_t));
	}

	{
		SCOPE_PROFILE("Bloom setup");

		constexpr uint32_t MIPS = 8;
		glm::ivec2 mipSize(1024);
		s_Data.BloomFBO = std::make_shared<Framebuffer>(1);
		for (uint32_t i = 0; i < MIPS; i++)
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

		ShaderSpec spec{};
		spec.Vertex   = { "resources/shaders/ScreenQuad.vert", {} };
		spec.Fragment = { "resources/shaders/BloomDownsampler.frag", {} };
		s_Data.BloomDownsamplerShader = std::make_shared<Shader>(spec);
		s_Data.BloomDownsamplerShader->Bind();
		s_Data.BloomDownsamplerShader->SetUniform1i("u_SourceTexture", 0);

		spec.Vertex   = { "resources/shaders/ScreenQuad.vert", {} };
		spec.Fragment = { "resources/shaders/BloomUpsampler.frag", {} };
		s_Data.BloomUpsamplerShader = std::make_shared<Shader>(spec);
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

		ShaderSpec spec{};
		spec.Vertex   = { "resources/shaders/ScreenQuad.vert", {} };
		spec.Fragment = { "resources/shaders/env_map/BRDF.frag", {} };
		std::shared_ptr<Shader> brdfShader = std::make_shared<Shader>(spec);
		brdfShader->Bind();
		Clear();
		DrawArrays(brdfShader, s_Data.ScreenQuadVertexArray, 6);
		s_Data.BRDF_Map = std::make_shared<Texture>((uint32_t)brdfTex, "BRDF Map", TextureFormat::RG16F);

		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		GLCall(glDeleteRenderbuffers(1, &rbo));
		GLCall(glDeleteFramebuffers(1, &fbo));
	}

	{
		SCOPE_PROFILE("Random offset texture");

		std::default_random_engine eng{};
		std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

		constexpr int32_t windowSize = 16;
		constexpr int32_t filterSize = 8;
		constexpr int32_t bufferSize = windowSize * windowSize * filterSize * filterSize * 2;

		std::vector<float> textureData{};
		textureData.resize(bufferSize);

		size_t idx = 0;
		for (int32_t texY = 0; texY < windowSize; texY++)
		{
			for (int32_t texX = 0; texX < windowSize; texX++)
			{
				for (int32_t v = filterSize - 1; v >= 0; v--)
				{
					for (int32_t u = 0; u < filterSize; u++)
					{
						assert(idx + 1 < textureData.size());

						float x = ((float)u + 0.5f + dist(eng)) / (float)filterSize;
						float y = ((float)v + 0.5f + dist(eng)) / (float)filterSize;

						textureData[idx	+ 0] = glm::sqrt(y) * glm::cos(glm::two_pi<float>() * x);
						textureData[idx + 1] = glm::sqrt(y) * glm::sin(glm::two_pi<float>() * x);
						idx += 2;
					}
				}
			}
		}

		constexpr int32_t filterSamples = filterSize * filterSize;
		GLCall(glGenTextures(1, &s_Data.OffsetsTexID));
		GLCall(glBindTexture(GL_TEXTURE_3D, s_Data.OffsetsTexID));
		GLCall(glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, filterSamples / 2, windowSize, windowSize));
		GLCall(glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, filterSamples / 2, windowSize, windowSize, GL_RGBA, GL_FLOAT, textureData.data()));
		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GLCall(glBindTexture(GL_TEXTURE_3D, 0));
	}

	{
		SCOPE_PROFILE("Deferred G buffer + shaders init");
		
		{
			const WindowSpec& wSpec = Application::Instance()->Spec();
			ColorAttachmentSpec spec;
			spec.Type = ColorAttachmentType::TEX_2D;
			spec.Format = TextureFormat::RGBA16F;
			spec.Wrap = GL_REPEAT;
			spec.MinFilter = GL_LINEAR;
			spec.MagFilter = GL_LINEAR;
			spec.BorderColor = glm::vec4(1.0f);
			spec.Size = { static_cast<int32_t>(wSpec.Width * 0.6f), wSpec.Height };
			s_Data.G_FBO = std::make_unique<Framebuffer>();
			s_Data.G_FBO->AddColorAttachment(spec);	// gPosition

			spec.Format = TextureFormat::RGB16F;
			s_Data.G_FBO->AddColorAttachment(spec);	// gNormal

			spec.Format = TextureFormat::RGBA8;
			s_Data.G_FBO->AddColorAttachment(spec);	// gColor

			spec.Format = TextureFormat::RGB8;
			s_Data.G_FBO->AddColorAttachment(spec);	// gMaterial
			s_Data.G_FBO->FillDrawBuffers();

			s_Data.G_FBO->AddRenderbuffer({
				.Type = RenderbufferType::DEPTH,
				.Size = { static_cast<int32_t>(wSpec.Width * 0.6f), wSpec.Height }
			});
			assert(s_Data.G_FBO->IsComplete() && "Incomplete framebuffer!");
		}

		{
			ShaderSpec spec{};
			spec.Vertex = {
				"resources/shaders/deferred/GBuf.vert", {}
			};
			spec.Fragment = {
				"resources/shaders/deferred/GBuf.frag",
				{
					{ "${MATERIALS_COUNT}",		std::to_string(s_Data.MaxMaterials)			 },
					{ "${TEXTURE_UNITS}",		std::to_string(s_Data.Specs.MaxTextureUnits) }
				}
			};
			s_Data.G_PassShader = std::make_shared<Shader>(spec);
			s_Data.G_PassShader->Bind();
			for (int32_t i = 0; i < s_Data.TextureBindings.size(); i++)
			{
				s_Data.G_PassShader->SetUniform1i("u_Textures[" + std::to_string(i) + "]", i);
			}
		}
		
		{
			ShaderSpec spec{};
			spec.Vertex		= { "resources/shaders/deferred/LightPass.vert", {} };
			spec.Fragment	= { "resources/shaders/deferred/LightPass.frag",
				{
					{ "${MAX_DIR_LIGHTS}",	 std::to_string(s_Data.MaxDirLights)	},
					{ "${MAX_POINT_LIGHTS}", std::to_string(s_Data.MaxPointLights)	},
					{ "${MAX_SPOTLIGHTS}",	 std::to_string(s_Data.MaxSpotlights)	},
					{ "${CASCADES_COUNT}",	 std::to_string(s_Data.CascadesCount)	}
				}
			};
			s_Data.G_LightShader = std::make_shared<Shader>(spec);
			s_Data.G_LightShader->Bind();
			s_Data.G_LightShader->SetUniform1i("gPosition", 0);
			s_Data.G_LightShader->SetUniform1i("gNormal", 1);
			s_Data.G_LightShader->SetUniform1i("gColor", 2);
			s_Data.G_LightShader->SetUniform1i("gMaterial", 3);
			s_Data.G_LightShader->SetUniform1i("u_DirLightCSM", s_Data.CSM_Slot);
			s_Data.G_LightShader->SetUniform1i("u_PointLightShadowmaps", s_Data.PointShadowSlot);
			s_Data.G_LightShader->SetUniform1i("u_SpotlightShadowmaps", s_Data.SpotlightShadowSlot);
			s_Data.G_LightShader->SetUniform1i("u_OffsetsTexSize", 16);
			s_Data.G_LightShader->SetUniform1i("u_OffsetsFilterSize", 8);
			s_Data.G_LightShader->SetUniform1f("u_OffsetsRadius", s_Data.OffsetsRadius);
			s_Data.G_LightShader->SetUniform1i("u_OffsetsTexture", s_Data.OffsetsSlot);
		}
	}

	memset(&s_Data.Stats, 0, sizeof(RendererStats));
}

void Renderer::Shutdown()
{
	s_TargetFBO = nullptr;

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
	s_Data.FlatShader = nullptr;
	s_Data.CurrentShader = nullptr;

	s_Data.ShadowMapsFBO = nullptr;
	s_Data.DirectionalShadowShader = nullptr;
	s_Data.PointShadowShader = nullptr;
	s_Data.SpotlightShadowShader = nullptr;

	s_Data.CameraBuffer = nullptr;
	s_Data.DirLightsBuffer = nullptr;
	s_Data.PointLightsBuffer = nullptr;
	s_Data.SpotlightsBuffer = nullptr;
	s_Data.MaterialsBuffer = nullptr;

	s_Data.G_FBO = nullptr;
	s_Data.G_PassShader = nullptr;
	s_Data.G_LightShader = nullptr;

	if (s_Data.OffsetsTexID != 0)
	{
		GLCall(glBindTexture(GL_TEXTURE_3D, 0));
		GLCall(glDeleteTextures(1, &s_Data.OffsetsTexID));
		s_Data.OffsetsTexID = 0;
	}
}

void Renderer::ReloadShaders()
{
	
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

	CameraBufferData cbd{};
	cbd.Projection = camera.GetProjection();
	cbd.View = camera.GetViewMatrix();
	cbd.Position = glm::vec4(camera.Position, 1.0f);
	cbd.Exposure = camera.Exposure;
	cbd.Gamma = camera.Gamma;
	cbd.Near = camera.m_NearClip;
	cbd.Far = camera.m_FarClip;

	s_Data.CameraBuffer->Bind();
	s_Data.CameraBuffer->SetData(&cbd, sizeof(CameraBufferData));

	StartBatch();
}

void Renderer::SceneEnd()
{
	Flush();
}

void Renderer::Flush()
{
	std::array<float, s_Data.CascadesCount> cascades = {
		s_ActiveCamera->m_FarClip / 50.0f,
		s_ActiveCamera->m_FarClip / 25.0f,
		s_ActiveCamera->m_FarClip / 10.0f,
		s_ActiveCamera->m_FarClip / 2.0f,
		s_ActiveCamera->m_FarClip
	};
	s_Data.DefaultShader->Bind();
	for (size_t i = 0; i < cascades.size(); i++)
	{
		s_Data.DefaultShader->SetUniform1f("u_CascadeDistances[" + std::to_string(i) + "]", cascades[i]);
	}
	s_Data.G_LightShader->Bind();
	for (size_t i = 0; i < cascades.size(); i++)
	{
		s_Data.G_LightShader->SetUniform1f("u_CascadeDistances[" + std::to_string(i) + "]", cascades[i]);
	}

	s_Data.MaterialsBuffer->SetData(s_Data.MaterialsData.data(), s_Data.MaterialsData.size() * sizeof(MaterialsBufferData));
	s_Data.ShadowMapsFBO->BindColorAttachment(0, s_Data.CSM_Slot);
	s_Data.ShadowMapsFBO->BindColorAttachment(1, s_Data.PointShadowSlot);
	s_Data.ShadowMapsFBO->BindColorAttachment(2, s_Data.SpotlightShadowSlot);

	uint32_t count = s_Data.DirLightsData.size();
	uint32_t offset = s_Data.MaxDirLights * sizeof(DirLightBufferData);
	s_Data.DirLightsBuffer->SetData(s_Data.DirLightsData.data(), s_Data.DirLightsData.size() * sizeof(DirLightBufferData));
	s_Data.DirLightsBuffer->SetData(&count, sizeof(int32_t), offset);

	count = s_Data.PointLightsData.size();
	offset = s_Data.MaxPointLights * sizeof(PointLightBufferData);
	s_Data.PointLightsBuffer->SetData(s_Data.PointLightsData.data(), s_Data.PointLightsData.size() * sizeof(PointLightBufferData));
	s_Data.PointLightsBuffer->SetData(&count, sizeof(int32_t), offset);

	count = s_Data.SpotlightsData.size();
	offset = s_Data.MaxSpotlights * sizeof(SpotlightBufferData);
	s_Data.SpotlightsBuffer->SetData(s_Data.SpotlightsData.data(), s_Data.SpotlightsData.size() * sizeof(SpotlightBufferData));
	s_Data.SpotlightsBuffer->SetData(&count, sizeof(int32_t), offset);

	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + i));
		GLCall(glBindTexture(GL_TEXTURE_2D, s_Data.TextureBindings[i]));
	}

	GLCall(glActiveTexture(GL_TEXTURE0 + s_Data.OffsetsSlot));
	GLCall(glBindTexture(GL_TEXTURE_3D, s_Data.OffsetsTexID));
	
	switch (s_Data.RenderMode)
	{
	case RenderMode::FORWARD:
		s_Data.CurrentShader = s_Data.DefaultShader;
		ForwardRender();
		break;
	case RenderMode::FLAT_SHADING:
		s_Data.CurrentShader = s_Data.FlatShader;
		ForwardRender();
		break;
	case RenderMode::DEFERRED:
		DeferredRender();
		break;
	default:
		assert(false && "Invalid rendering pipeline passed");
		break;
	}
}

void Renderer::BeginShadowMapPass()
{
	StartBatch();
}

void Renderer::EndShadowMapPass()
{
	uint32_t count = s_Data.DirLightsData.size();
	uint32_t offset = s_Data.MaxDirLights * sizeof(DirLightBufferData);
	s_Data.DirLightsBuffer->SetData(s_Data.DirLightsData.data(), s_Data.DirLightsData.size() * sizeof(DirLightBufferData));
	s_Data.DirLightsBuffer->SetData(&count, sizeof(int32_t), offset);

	count = s_Data.PointLightsData.size();
	offset = s_Data.MaxPointLights * sizeof(PointLightBufferData);
	s_Data.PointLightsBuffer->SetData(s_Data.PointLightsData.data(), s_Data.PointLightsData.size() * sizeof(PointLightBufferData));
	s_Data.PointLightsBuffer->SetData(&count, sizeof(int32_t), offset);

	count = s_Data.SpotlightsData.size();
	offset = s_Data.MaxSpotlights * sizeof(SpotlightBufferData);
	s_Data.SpotlightsBuffer->SetData(s_Data.SpotlightsData.data(), s_Data.SpotlightsData.size() * sizeof(SpotlightBufferData));
	s_Data.SpotlightsBuffer->SetData(&count, sizeof(int32_t), offset);

	for (auto& [meshID, meshData] : s_Data.MeshesData)
	{
		if (meshData.CurrentInstancesCount == 0)
		{
			continue;
		}
		
		Mesh& mesh = AssetManager::GetMesh(meshID);
		mesh.InstanceBuffer->SetData(meshData.Instances.data(), meshData.CurrentInstancesCount * sizeof(MeshInstance));
	}

	s_Data.ShadowMapsFBO->Bind();
	s_Data.ShadowMapsFBO->DrawToDepthMap(0);

	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
	// GLCall(glCullFace(GL_FRONT));

	Clock cock;
	cock.Restart();
	if (!s_Data.DirLightsData.empty())
	{
		for (auto& [meshID, meshData] : s_Data.MeshesData)
		{
			if (meshData.CurrentInstancesCount == 0)
			{
				continue;
			}

			Mesh& mesh = AssetManager::GetMesh(meshID);
			DrawIndexedInstanced(s_Data.DirectionalShadowShader, mesh.VAO, meshData.CurrentInstancesCount);
		}
		GLCall(glFinish());
	}
	s_Data.Stats.DirLightShadowPassTime = cock.GetElapsedTime();

	s_Data.ShadowMapsFBO->DrawToDepthMap(1);
	// GLCall(glCullFace(GL_BACK));
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
	cock.Restart();
	if (!s_Data.PointLightsData.empty())
	{
		for (auto& [meshID, meshData] : s_Data.MeshesData)
		{
			if (meshData.CurrentInstancesCount == 0)
			{
				continue;
			}

			Mesh& mesh = AssetManager::GetMesh(meshID);
			DrawIndexedInstanced(s_Data.PointShadowShader, mesh.VAO, meshData.CurrentInstancesCount);
		}
		GLCall(glFinish());
	}
	s_Data.Stats.PointLightShadowPassTime = cock.GetElapsedTime();

	s_Data.ShadowMapsFBO->DrawToDepthMap(2);
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
	cock.Restart();
	if (!s_Data.SpotlightsData.empty())
	{
		for (auto& [meshID, meshData] : s_Data.MeshesData)
		{
			if (meshData.CurrentInstancesCount == 0)
			{
				continue;
			}

			Mesh& mesh = AssetManager::GetMesh(meshID);
			DrawIndexedInstanced(s_Data.SpotlightShadowShader, mesh.VAO, meshData.CurrentInstancesCount);
		}
		GLCall(glFinish());
	}
	s_Data.Stats.SpotlightShadowPassTime = cock.GetElapsedTime();

	s_Data.Stats.DirLightCascadesPassed = 0;
	s_Data.Stats.PointLightFacesShadowPassed = 0;
	s_Data.Stats.SpotlightFacesShadowPassed = 0;
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

void Renderer::SetRenderMode(RenderMode mode)
{
	s_Data.RenderMode = mode;
}

void Renderer::SetTargetFBO(std::shared_ptr<Framebuffer>& fbo)
{
	s_TargetFBO = fbo;
}

void Renderer::Bloom(std::shared_ptr<Framebuffer> hdrFBO)
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

void Renderer::SetBloomThreshold(float threshold)
{
	s_Data.BloomDownsamplerShader->Bind();
	s_Data.BloomDownsamplerShader->SetUniform1f("u_Threshold", threshold);
}

void Renderer::SetOffsetsRadius(float radius)
{
	s_Data.OffsetsRadius = radius;
	
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform1f("u_OffsetsRadius", radius);
}

std::shared_ptr<Framebuffer> Renderer::CreateEnvCubemap(std::shared_ptr<Texture> hdrEnvMap, const glm::uvec2& faceSize)
{
	// Mapping env map to a cubemap
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
	s_Data.CameraBuffer->Bind();
	s_Data.CameraBuffer->SetData(glm::value_ptr(captureProj), sizeof(glm::mat4));

	cfb->Bind();
	cfb->BindRenderbuffer();
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));

	hdrEnvMap->Bind();
	s_Data.EnvMapShader->Bind();
	for (uint32_t i = 0; i < 6; i++)
	{
		s_Data.CameraBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
		cfb->DrawToCubeColorAttachment(0, 0, i);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawArrays(s_Data.EnvMapShader, s_Data.EnvMapVertexArray, 36);
	}
	cfb->BindColorAttachment(0);
	GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

	// Irradiance map
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
		s_Data.CameraBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
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

	// Reflection maps
	cfb->BindColorAttachment(0);
	s_Data.PrefilterShader->Bind();
	cfb->ResizeRenderbuffer({ 128, 128 });
	s_Data.PrefilterShader->SetUniform1f("u_Roughness", 0.0f);
	for (uint32_t i = 0; i < 6; i++)
	{
		s_Data.CameraBuffer->SetData(glm::value_ptr(captureViews[i]), sizeof(glm::mat4), sizeof(glm::mat4));
		cfb->DrawToCubeColorAttachment(2, 0, i);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawArrays(s_Data.PrefilterShader, s_Data.EnvMapVertexArray, 36);
	}
	GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
	
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

	cfb->BindColorAttachment(1, s_Data.IrradianceSlot);
	cfb->BindColorAttachment(2, s_Data.PrefilterSlot);
	s_Data.BRDF_Map->Bind(s_Data.BRDF_Slot);
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform1i("u_IrradianceMap", s_Data.IrradianceSlot);
	s_Data.DefaultShader->SetUniform1i("u_PrefilterMap", s_Data.PrefilterSlot);
	s_Data.DefaultShader->SetUniform1i("u_BRDF_LUT", s_Data.BRDF_Slot);

	s_Data.G_LightShader->Bind();
	s_Data.G_LightShader->SetUniform1i("u_IrradianceMap", s_Data.IrradianceSlot);
	s_Data.G_LightShader->SetUniform1i("u_PrefilterMap", s_Data.PrefilterSlot);
	s_Data.G_LightShader->SetUniform1i("u_BRDF_LUT", s_Data.BRDF_Slot);
}

void Renderer::AddDirectionalLight(const TransformComponent& transform, const DirectionalLightComponent& light)
{
	if (s_Data.DirLightsData.size() >= s_Data.MaxDirLights)
	{
		return;
	}
	
	std::array<glm::mat4, s_Data.CascadesCount> cascadedMatrices{};
	glm::vec3 dir = glm::toMat3(glm::quat(transform.Rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
	std::array<float, s_Data.CascadesCount> nearPlanes = {
		s_ActiveCamera->m_NearClip,
		s_ActiveCamera->m_FarClip / 50.0f,
		s_ActiveCamera->m_FarClip / 25.0f,
		s_ActiveCamera->m_FarClip / 10.0f,
		s_ActiveCamera->m_FarClip / 2.0f
	};
	std::array<float, s_Data.CascadesCount> farPlanes = {
		nearPlanes[1],
		nearPlanes[2],
		nearPlanes[3],
		nearPlanes[4],
		s_ActiveCamera->m_FarClip
	};

	for (size_t i = 0; i < nearPlanes.size(); i++)
	{
		glm::mat4 proj = glm::perspective(glm::radians(s_ActiveCamera->m_FOV), s_ActiveCamera->m_AspectRatio, nearPlanes[i], farPlanes[i]);
		std::vector<glm::vec4> viewCorners = FrustumCornersWorldSpace(proj * s_ActiveCamera->GetViewMatrix());
		glm::vec3 center(0.0f);
		for (const glm::vec4& corner : viewCorners)
		{
			center += glm::vec3(corner);
		}
		center /= viewCorners.size();

		glm::mat4 lightView = glm::lookAt(center + dir, center, glm::vec3(0.0f, 1.0f, 0.0f));
		float minX =  FLT_MAX;
		float maxX = -FLT_MAX;
		float minY =  FLT_MAX;
		float maxY = -FLT_MAX;
		float minZ =  FLT_MAX;
		float maxZ = -FLT_MAX;
		for (const glm::vec4& corner : viewCorners)
		{
			glm::vec4 trf = lightView * corner;
			minX = glm::min(minX, trf.x);
			maxX = glm::max(maxX, trf.x);
			minY = glm::min(minY, trf.y);
			maxY = glm::max(maxY, trf.y);
			minZ = glm::min(minZ, trf.z);
			maxZ = glm::max(maxZ, trf.z);
		}

		constexpr float Z_MULT = 10.0f;
		minZ = (minZ < 0.0f ? minZ * Z_MULT : minZ / Z_MULT);
		maxZ = (maxZ < 0.0f ? maxZ / Z_MULT : maxZ * Z_MULT);

		glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
		cascadedMatrices[i] = lightProj * lightView;
	}

	s_Data.DirLightsData.push_back({
		cascadedMatrices,
		glm::vec4(dir, 1.0f),
		glm::vec4(light.Color * light.Intensity, 1.0f) 
	});
	s_Data.Stats.DirLightCascadesPassed += s_Data.CascadesCount;
}

void Renderer::AddPointLight(const glm::vec3& position, const PointLightComponent& light)
{
	if (s_Data.PointLightsData.size() >= s_Data.MaxPointLights)
	{
		return;
	}

	float radius = LightRadius(1.0f, light.LinearTerm, light.QuadraticTerm, MaxComponent(light.Color * light.Intensity));
	glm::mat4 proj = glm::perspective(glm::radians(91.0f), 1.0f, 0.1f, radius);
	std::array<glm::vec4, 6> dirs = {
		glm::vec4( 1.0f,  0.0f,  0.0f,  0.0f),
		glm::vec4(-1.0f,  0.0f,  0.0f,  0.0f),
		glm::vec4( 0.0f,  1.0f,  0.0f,  0.0f),
		glm::vec4( 0.0f, -1.0f,  0.0f,  0.0f),
		glm::vec4( 0.0f,  0.0f,  1.0f,  0.0f),
		glm::vec4( 0.0f,  0.0f, -1.0f,  0.0f)
	};
	std::array<glm::mat4, 6> lightSpaceMatrices = {
		proj * glm::lookAt(position, position + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f)),
		proj * glm::lookAt(position, position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f)),
		proj * glm::lookAt(position, position + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3( 0.0f,  0.0f,  1.0f)),
		proj * glm::lookAt(position, position + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f)),
		proj * glm::lookAt(position, position + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3( 0.0f, -1.0f,  0.0f)),
		proj * glm::lookAt(position, position + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3( 0.0f, -1.0f,  0.0f))
	};
	s_Data.PointLightsData.push_back({
		lightSpaceMatrices,
		dirs,
		glm::vec4(position, light.LinearTerm), 
		glm::vec4(light.Color * light.Intensity, light.QuadraticTerm) ,
		6
	});

	s_Data.Stats.PointLightFacesShadowPassed += 6;
}

void Renderer::AddSpotLight(const TransformComponent& transform, const SpotLightComponent& light)
{
	if (s_Data.SpotlightsData.size() >= s_Data.MaxSpotlights)
	{
		return;
	}

	float radius = LightRadius(1.0f, light.LinearTerm, light.QuadraticTerm, MaxComponent(light.Color * light.Intensity));
	glm::vec3 dir = glm::toMat3(glm::quat(transform.Rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
	glm::mat4 proj = glm::perspective(glm::radians(2.0f * light.Cutoff), 1.0f, 0.1f, radius);
	glm::mat4 view = glm::lookAt(transform.Position, transform.Position + dir, glm::vec3(0.0, 1.0f, 0.0f));

	s_Data.SpotlightsData.push_back({
		proj * view,
		glm::vec4(transform.Position, glm::cos(glm::radians(light.Cutoff))),
		glm::vec4(dir, glm::cos(glm::radians(light.Cutoff - light.EdgeSmoothness))),
		glm::vec4(light.Color * light.Intensity, light.LinearTerm),
		light.QuadraticTerm
	});
	s_Data.Stats.SpotlightFacesShadowPassed++;
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

	s_Data.MaterialsData.clear();
	s_Data.DirLightsData.clear();
	s_Data.PointLightsData.clear();
	s_Data.SpotlightsData.clear();

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

void Renderer::ForwardRender()
{
	for (auto& [meshID, meshData] : s_Data.MeshesData)
	{
		if (meshData.CurrentInstancesCount == 0)
		{
			continue;
		}

		Mesh& mesh = AssetManager::GetMesh(meshID);
		mesh.InstanceBuffer->SetData(meshData.Instances.data(), meshData.CurrentInstancesCount * sizeof(MeshInstance));
		DrawIndexedInstanced(s_Data.CurrentShader, mesh.VAO, meshData.CurrentInstancesCount);
		s_Data.Stats.RenderPassDrawCalls++;
	}

	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineBufferPtr - (uint8_t*)s_Data.LineBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineBufferBase, dataSize);

		GLCall(glDisable(GL_CULL_FACE));
		DrawArrays(s_Data.LineShader, s_Data.LineVertexArray, s_Data.LineVertexCount, GL_LINES);
		GLCall(glEnable(GL_CULL_FACE));
		s_Data.Stats.RenderPassDrawCalls++;
	}
}

void Renderer::DeferredRender()
{
	s_Data.G_FBO->Bind();
	s_Data.G_FBO->BindRenderbuffer();
	s_Data.G_FBO->DrawToColorAttachment(0, 0);
	s_Data.G_FBO->DrawToColorAttachment(1, 1);
	s_Data.G_FBO->DrawToColorAttachment(2, 2);
	s_Data.G_FBO->DrawToColorAttachment(3, 3);
	s_Data.G_FBO->FillDrawBuffers();
	Renderer::ClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	Renderer::Clear();
	
	for (auto& [meshID, meshData] : s_Data.MeshesData)
	{
		if (meshData.CurrentInstancesCount == 0)
		{
			continue;
		}

		Mesh& mesh = AssetManager::GetMesh(meshID);
		mesh.InstanceBuffer->SetData(meshData.Instances.data(), meshData.CurrentInstancesCount * sizeof(MeshInstance));
		DrawIndexedInstanced(s_Data.G_PassShader, mesh.VAO, meshData.CurrentInstancesCount);
	}
	
	s_Data.G_FBO->BindColorAttachment(0, 0);
	s_Data.G_FBO->BindColorAttachment(1, 1);
	s_Data.G_FBO->BindColorAttachment(2, 2);
	s_Data.G_FBO->BindColorAttachment(3, 3);
	s_Data.G_FBO->BlitRenderbuffer(s_TargetFBO);
	
	s_TargetFBO->Bind();
	s_TargetFBO->BindRenderbuffer();
	s_TargetFBO->DrawToColorAttachment(0, 0);
	s_TargetFBO->DrawToColorAttachment(1, 1);
	s_TargetFBO->FillDrawBuffers();
	DrawArrays(s_Data.G_LightShader, s_Data.ScreenQuadVertexArray, 6);
}
