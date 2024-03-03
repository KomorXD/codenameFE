#include "Renderer.hpp"
#include "OpenGL.hpp"
#include "../Logger.hpp"
#include "../Timer.hpp"
#include "Camera.hpp"
#include "PrimitivesGen.hpp"

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

struct QuadInstance
{
	glm::mat4 Transform;
	glm::vec4 Color;
	float	  TextureSlot;
};

struct CubeInstance
{
	glm::mat4 Transform;
	glm::vec4 Color;
	float	  TextureSlot;
};

struct RendererData
{
	static constexpr uint32_t MaxQuads	   = 5000;
	static constexpr uint32_t MaxVertices  = MaxQuads * 4;
	static constexpr uint32_t MaxIndices   = MaxQuads * 6;
	static constexpr uint32_t MaxInstances = MaxQuads / 4;

	std::shared_ptr<VertexArray>  ScreenQuadVertexArray;
	std::shared_ptr<VertexBuffer> ScreenQuadVertexBuffer;
	std::shared_ptr<Shader>		  ScreenQuadShader;

	std::shared_ptr<VertexArray>  LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;
	std::shared_ptr<Shader>		  LineShader;
	uint32_t	LineVertexCount = 0;
	LineVertex* LineBufferBase  = nullptr;
	LineVertex* LineBufferPtr   = nullptr;

	std::shared_ptr<VertexArray>  QuadVertexArray;
	std::shared_ptr<VertexBuffer> QuadVertexBuffer;
	std::shared_ptr<VertexBuffer> QuadInstanceBuffer;
	uint32_t	  QuadInstanceCount = 0;
	QuadInstance* QuadBufferBase    = nullptr;
	QuadInstance* QuadBufferPtr     = nullptr;

	std::shared_ptr<VertexArray>  CubeVertexArray;
	std::shared_ptr<VertexBuffer> CubeVertexBuffer;
	std::shared_ptr<VertexBuffer> CubeInstanceBuffer;
	uint32_t	  CubeInstanceCount = 0;
	CubeInstance* CubeBufferBase	= nullptr;
	CubeInstance* CubeBufferPtr	    = nullptr;

	std::shared_ptr<Shader>	DefaultShader;
	std::shared_ptr<Shader> DepthShader;
	std::shared_ptr<Shader> CurrentShader;
	
	uint32_t DirLightsCount   = 0;
	uint32_t PointLightsCount = 0;
	uint32_t SpotLightsCount  = 0;

	std::shared_ptr<UniformBuffer> MatricesBuffer;

	uint32_t BoundTexturesCount = 1;
	std::array<int32_t, 24> TextureBindings;
	std::shared_ptr<Texture> DefaultAlbedo;
	
};

static RendererData s_Data{};

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
		SCOPE_PROFILE("Quad data init");

		auto [vertices, indices] = QuadMeshData();
		std::unique_ptr<IndexBuffer> ibo = std::make_unique<IndexBuffer>(indices.data(), (uint32_t)indices.size());
		s_Data.QuadVertexArray = std::make_shared<VertexArray>();
		s_Data.QuadVertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex), vertices.size());

		VertexBufferLayout layout;
		layout.Push<float>(3); // 0 Position
		layout.Push<float>(3); // 1 Normal
		layout.Push<float>(2); // 2 Texture UV
		s_Data.QuadVertexArray->AddBuffers(s_Data.QuadVertexBuffer, ibo, layout);

		layout.Clear();
		layout.Push<float>(4); // 3 Transform
		layout.Push<float>(4); // 4 Transform
		layout.Push<float>(4); // 5 Transform
		layout.Push<float>(4); // 6 Transform
		layout.Push<float>(4); // 7 Color
		layout.Push<float>(1); // 8 Texture slot
		s_Data.QuadInstanceBuffer = std::make_shared<VertexBuffer>(nullptr, s_Data.MaxInstances * sizeof(CubeInstance));
		s_Data.QuadVertexArray->AddInstancedVertexBuffer(s_Data.QuadInstanceBuffer, layout, 3);

		s_Data.QuadBufferBase = new QuadInstance[s_Data.MaxInstances];
	}

	{
		SCOPE_PROFILE("Cube data init");

		auto [vertices, indices] = CubeMeshData();
		std::unique_ptr<IndexBuffer> ibo = std::make_unique<IndexBuffer>(indices.data(), (uint32_t)indices.size());
		s_Data.CubeVertexArray = std::make_shared<VertexArray>();
		s_Data.CubeVertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex), vertices.size());

		VertexBufferLayout layout;
		layout.Push<float>(3); // 0 Position
		layout.Push<float>(3); // 1 Normal
		layout.Push<float>(2); // 2 Texture UV
		s_Data.CubeVertexArray->AddBuffers(s_Data.CubeVertexBuffer, ibo, layout);

		layout.Clear();
		layout.Push<float>(4); // 3 Transform
		layout.Push<float>(4); // 4 Transform
		layout.Push<float>(4); // 5 Transform
		layout.Push<float>(4); // 6 Transform
		layout.Push<float>(4); // 7 Color
		layout.Push<float>(1); // 8 Texture slot
		s_Data.CubeInstanceBuffer = std::make_shared<VertexBuffer>(nullptr, s_Data.MaxInstances * sizeof(CubeInstance));
		s_Data.CubeVertexArray->AddInstancedVertexBuffer(s_Data.CubeInstanceBuffer, layout, 3);

		s_Data.CubeBufferBase = new CubeInstance[s_Data.MaxInstances];
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

		uint8_t whitePixel[] = { 255.0f, 255.0f, 255.0f, 255.0f };
		s_Data.DefaultAlbedo = std::make_shared<Texture>(whitePixel, 1, 1);

		s_Data.DepthShader = std::make_shared<Shader>("resources/shaders/DefaultDepth.vert", "resources/shaders/DefaultDepth.frag");
	}
}

void Renderer::Shutdown()
{
	delete[] s_Data.QuadBufferBase;
	delete[] s_Data.LineBufferBase;
	delete[] s_Data.CubeBufferBase;
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
	s_Data.DefaultAlbedo->Bind();
	for (int32_t i = 1; i < s_Data.BoundTexturesCount; i++)
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + i));
		GLCall(glBindTexture(GL_TEXTURE_2D, s_Data.TextureBindings[i]));
	}

	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineBufferPtr - (uint8_t*)s_Data.LineBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineBufferBase, dataSize);

		GLCall(glDisable(GL_CULL_FACE));
		DrawArrays(s_Data.LineShader, s_Data.LineVertexArray, s_Data.LineVertexCount, GL_LINES);
		GLCall(glEnable(GL_CULL_FACE));
	}

	if (s_Data.QuadInstanceCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadBufferPtr - (uint8_t*)s_Data.QuadBufferBase);
		s_Data.QuadInstanceBuffer->SetData(s_Data.QuadBufferBase, dataSize);

		s_Data.DefaultShader->Bind();
		s_Data.DefaultShader->SetUniform1i("u_DirLightsCount",   s_Data.DirLightsCount);
		s_Data.DefaultShader->SetUniform1i("u_PointLightsCount", s_Data.PointLightsCount);
		s_Data.DefaultShader->SetUniform1i("u_SpotLightsCount",  s_Data.SpotLightsCount);

		DrawIndexedInstanced(s_Data.CurrentShader, s_Data.QuadVertexArray, s_Data.QuadInstanceCount);
	}

	if (s_Data.CubeInstanceCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeBufferPtr - (uint8_t*)s_Data.CubeBufferBase);
		s_Data.CubeInstanceBuffer->SetData(s_Data.CubeBufferBase, dataSize);

		s_Data.DefaultShader->Bind();
		s_Data.DefaultShader->SetUniform1i("u_DirLightsCount",   s_Data.DirLightsCount);
		s_Data.DefaultShader->SetUniform1i("u_PointLightsCount", s_Data.PointLightsCount);
		s_Data.DefaultShader->SetUniform1i("u_SpotLightsCount",  s_Data.SpotLightsCount);

		DrawIndexedInstanced(s_Data.CurrentShader, s_Data.CubeVertexArray, s_Data.CubeInstanceCount);
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

void Renderer::DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
{
	if (s_Data.QuadInstanceCount >= s_Data.MaxInstances)
	{
		NextBatch();
	}

	s_Data.QuadBufferPtr->Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
	s_Data.QuadBufferPtr->Color = color;
	s_Data.QuadBufferPtr->TextureSlot = 0.0f;

	++s_Data.QuadBufferPtr;
	++s_Data.QuadInstanceCount;
}

void Renderer::DrawQuad(const glm::vec3& position, const glm::vec3& size, const Texture& texture)
{
	if (s_Data.QuadInstanceCount >= 254 || s_Data.BoundTexturesCount >= s_Data.TextureBindings.size() - 1)
	{
		NextBatch();
	}

	s_Data.QuadBufferPtr->Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
	s_Data.QuadBufferPtr->Color = glm::vec4(1.0f);

	bool duplicateFound = false;
	int32_t duplicateIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == texture.GetID())
		{
			duplicateFound = true;
			duplicateIdx = i;
			break;
		}
	}

	if (duplicateFound)
	{
		s_Data.QuadBufferPtr->TextureSlot = (float)duplicateIdx;
	}
	else
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = texture.GetID();
		s_Data.QuadBufferPtr->TextureSlot = (float)s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	++s_Data.QuadBufferPtr;
	++s_Data.QuadInstanceCount;
}

void Renderer::DrawQuad(const glm::mat4& transform, const Material& material)
{
	if (s_Data.QuadInstanceCount >= 254 || s_Data.BoundTexturesCount >= s_Data.TextureBindings.size() - 1)
	{
		NextBatch();
	}

	s_Data.QuadBufferPtr->Transform = transform;
	s_Data.QuadBufferPtr->Color = material.Color;

	if (!material.AlbedoTexture)
	{
		s_Data.QuadBufferPtr->TextureSlot = 0.0f;
		++s_Data.QuadBufferPtr;
		++s_Data.QuadInstanceCount;

		return;
	}

	int32_t duplicateIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == material.AlbedoTexture->GetID())
		{
			duplicateIdx = i;
			break;
		}
	}

	if (duplicateIdx == -1)
	{
		s_Data.QuadBufferPtr->TextureSlot = (float)duplicateIdx;
	}
	else
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = material.AlbedoTexture->GetID();
		s_Data.QuadBufferPtr->TextureSlot = (float)s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	++s_Data.QuadBufferPtr;
	++s_Data.QuadInstanceCount;
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
{
	if (s_Data.CubeInstanceCount >= s_Data.MaxInstances)
	{
		NextBatch();
	}

	s_Data.CubeBufferPtr->Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
	s_Data.CubeBufferPtr->Color = color;
	s_Data.CubeBufferPtr->TextureSlot = 0.0f;

	++s_Data.CubeBufferPtr;
	++s_Data.CubeInstanceCount;
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const Texture& texture)
{
	if (s_Data.CubeInstanceCount >= 254 || s_Data.BoundTexturesCount >= s_Data.TextureBindings.size() - 1)
	{
		NextBatch();
	}

	s_Data.CubeBufferPtr->Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
	s_Data.CubeBufferPtr->Color = glm::vec4(1.0f);

	bool duplicateFound = false;
	int32_t duplicateIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == texture.GetID())
		{
			duplicateFound = true;
			duplicateIdx = i;
			break;
		}
	}

	if (duplicateFound)
	{
		s_Data.CubeBufferPtr->TextureSlot = (float)duplicateIdx;
	}
	else
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = texture.GetID();
		s_Data.CubeBufferPtr->TextureSlot = (float)s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	++s_Data.CubeBufferPtr;
	++s_Data.CubeInstanceCount;
}

void Renderer::DrawCube(const glm::mat4& transform, const Material& material)
{
	if (s_Data.CubeInstanceCount >= 254 || s_Data.BoundTexturesCount >= s_Data.TextureBindings.size() - 1)
	{
		NextBatch();
	}

	s_Data.CubeBufferPtr->Transform = transform;
	s_Data.CubeBufferPtr->Color = material.Color;

	if (!material.AlbedoTexture)
	{
		s_Data.CubeBufferPtr->TextureSlot = 0.0f;
		++s_Data.CubeBufferPtr;
		++s_Data.CubeInstanceCount;

		return;
	}
	
	int32_t duplicateIdx = -1;
	for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
	{
		if (s_Data.TextureBindings[i] == material.AlbedoTexture->GetID())
		{
			duplicateIdx = i;
			break;
		}
	}

	if (duplicateIdx == -1)
	{
		s_Data.CubeBufferPtr->TextureSlot = (float)duplicateIdx;
	}
	else
	{
		s_Data.TextureBindings[s_Data.BoundTexturesCount] = material.AlbedoTexture->GetID();
		s_Data.CubeBufferPtr->TextureSlot = (float)s_Data.BoundTexturesCount;
		++s_Data.BoundTexturesCount;
	}

	++s_Data.CubeBufferPtr;
	++s_Data.CubeInstanceCount;
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

void Renderer::AddDirectionalLight(const DirectionalLight& light)
{
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_DirLights[" + std::to_string(s_Data.DirLightsCount) + "].direction", glm::normalize(light.Direction));
	s_Data.DefaultShader->SetUniform3f("u_DirLights[" + std::to_string(s_Data.DirLightsCount) + "].color",	   light.Color);

	s_Data.DirLightsCount++;
}

void Renderer::AddPointLight(const glm::vec3& position, const PointLight& light)
{
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].position",	   position);
	s_Data.DefaultShader->SetUniform3f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].color",		   light.Color);
	s_Data.DefaultShader->SetUniform1f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].linearTerm",	   light.LinearTerm);
	s_Data.DefaultShader->SetUniform1f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].quadraticTerm", light.QuadraticTerm);

	s_Data.PointLightsCount++;
}

void Renderer::AddSpotLight(const glm::vec3& position, const SpotLight& light)
{
	s_Data.DefaultShader->Bind();
	s_Data.DefaultShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].position",	  position);
	s_Data.DefaultShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].cutOff",		  glm::cos(glm::radians(light.Cutoff)));
	s_Data.DefaultShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].direction",	  light.Direction);
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

void Renderer::RenderDefault()
{
	GLCall(glCullFace(GL_BACK));
	GLCall(glEnable(GL_CULL_FACE));
	s_Data.CurrentShader = s_Data.DefaultShader;
}

void Renderer::RenderDepth()
{
	GLCall(glCullFace(GL_FRONT));
	GLCall(glDisable(GL_CULL_FACE));
	s_Data.CurrentShader = s_Data.DepthShader;
}

Viewport Renderer::CurrentViewport()
{
	return s_Viewport;
}

void Renderer::StartBatch()
{
	s_Data.LineVertexCount = 0;
	s_Data.LineBufferPtr = s_Data.LineBufferBase;

	s_Data.QuadInstanceCount = 0;
	s_Data.QuadBufferPtr = s_Data.QuadBufferBase;

	s_Data.CubeInstanceCount = 0;
	s_Data.CubeBufferPtr = s_Data.CubeBufferBase;

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
