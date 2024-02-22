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

glm::mat4 Renderer::s_ViewProjection = glm::mat4(1.0f);
glm::mat4 Renderer::s_Projection = glm::mat4(1.0f);
glm::mat4 Renderer::s_View = glm::mat4(1.0f);

struct QuadVertex
{
	glm::vec3 Position;
	glm::vec4 Color;
};

struct LineVertex
{
	glm::vec3 Position;
	glm::vec4 Color;
};

struct CubeInstance
{
	glm::mat4 Transform;
	glm::vec4 Color;
	float	  TextureSlot;
};

struct RendererData
{
	static constexpr uint32_t MaxQuads = 5000;
	static constexpr uint32_t MaxVertices = MaxQuads * 4;
	static constexpr uint32_t MaxIndices = MaxQuads * 6;

	std::shared_ptr<VertexArray>  QuadVertexArray;
	std::shared_ptr<VertexBuffer> QuadVertexBuffer;
	std::shared_ptr<Shader>		  QuadShader;

	std::shared_ptr<VertexArray>  LineVertexArray;
	std::shared_ptr<VertexBuffer> LineVertexBuffer;
	std::shared_ptr<Shader>		  LineShader;

	std::shared_ptr<VertexArray>  CubeVertexArray;
	std::shared_ptr<VertexBuffer> CubeVertexBuffer;
	std::shared_ptr<VertexBuffer> CubeInstanceBuffer;
	std::shared_ptr<Shader>		  CubeShader;

	uint32_t	QuadIndexCount = 0;
	QuadVertex* QuadBufferBase = nullptr;
	QuadVertex* QuadBufferPtr  = nullptr;
	std::array<glm::vec4, 4> QuadVertices;

	uint32_t	LineVertexCount = 0;
	LineVertex* LineBufferBase  = nullptr;
	LineVertex* LineBufferPtr   = nullptr;

	uint32_t	  CubeInstanceCount = 0;
	CubeInstance* CubeBufferBase	= nullptr;
	CubeInstance* CubeBufferPtr	    = nullptr;
	
	uint32_t DirLightsCount   = 0;
	uint32_t PointLightsCount = 0;
	uint32_t SpotLightsCount  = 0;

	std::shared_ptr<UniformBuffer> MatricesBuffer;

	uint32_t BoundTexturesCount = 0;
	std::array<int32_t, 24> TextureBindings;
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
		SCOPE_PROFILE("Quad data init");

		s_Data.QuadVertexArray = std::make_shared<VertexArray>();
		s_Data.QuadVertexBuffer = std::make_shared<VertexBuffer>(nullptr, s_Data.MaxVertices * sizeof(QuadVertex));

		VertexBufferLayout layout;

		layout.Push<float>(3); // Position
		layout.Push<float>(4); // Color

		std::vector<uint32_t> quadIndices(s_Data.MaxIndices);

		uint32_t offset = 0;

		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		std::unique_ptr<IndexBuffer> ibo = std::make_unique<IndexBuffer>(quadIndices.data(), (uint32_t)quadIndices.size());
		s_Data.QuadVertexArray->AddBuffers(s_Data.QuadVertexBuffer, ibo, layout);
		s_Data.QuadBufferBase = new QuadVertex[s_Data.MaxVertices];
		s_Data.QuadShader = std::make_shared<Shader>("resources/shaders/Quad.vert", "resources/shaders/Quad.frag");

		s_Data.QuadVertices[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertices[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertices[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.QuadVertices[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
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
		s_Data.LineShader = std::make_shared<Shader>("resources/shaders/Quad.vert", "resources/shaders/Quad.frag");
	}

	{
		SCOPE_PROFILE("Cube data init");

		std::vector<CubeVertex> cubeVertices = CubeVertexData();

		s_Data.CubeVertexArray = std::make_shared<VertexArray>();
		s_Data.CubeVertexBuffer = std::make_shared<VertexBuffer>(cubeVertices.data(), cubeVertices.size() * sizeof(CubeVertex), cubeVertices.size());

		VertexBufferLayout layout;
		layout.Push<float>(3); // 0 Position
		layout.Push<float>(3); // 1 Normal
		layout.Push<float>(2); // 2 Texture UV
		s_Data.CubeVertexArray->AddVertexBuffer(s_Data.CubeVertexBuffer, layout);

		layout.Clear();
		layout.Push<float>(4); // 3 Transform
		layout.Push<float>(4); // 4 Transform
		layout.Push<float>(4); // 5 Transform
		layout.Push<float>(4); // 6 Transform
		layout.Push<float>(4); // 7 Color
		layout.Push<float>(1); // 8 Texture ID
		s_Data.CubeInstanceBuffer = std::make_shared<VertexBuffer>(nullptr, s_Data.MaxVertices * sizeof(CubeInstance));
		s_Data.CubeVertexArray->AddInstancedVertexBuffer(s_Data.CubeInstanceBuffer, layout, 3);

		s_Data.CubeBufferBase = new CubeInstance[255];
		s_Data.CubeShader = std::make_shared<Shader>("resources/shaders/Cube.vert", "resources/shaders/Cube.frag");
		s_Data.CubeShader->Bind();
		
		for (int32_t i = 0; i < s_Data.TextureBindings.size(); i++)
		{
			s_Data.CubeShader->SetUniform1i("u_Textures[" + std::to_string(i) + "]", i);
		}
	}

	{
		SCOPE_PROFILE("Uniform buffers init");

		s_Data.MatricesBuffer = std::make_shared<UniformBuffer>(nullptr, 2 * sizeof(glm::mat4));
		s_Data.MatricesBuffer->BindBufferRange(0, 0, 2 * sizeof(glm::mat4));
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
	s_Data.QuadShader->ReloadShader();
	s_Data.LineShader->ReloadShader();
	s_Data.CubeShader->ReloadShader();
}

void Renderer::OnWindowResize(const Viewport& newViewport)
{
	auto& [x, y, width, height] = newViewport;
	GLCall(glViewport(x, y, width, height));
}

void Renderer::SceneBegin(Camera& camera)
{
	s_ViewProjection = camera.GetViewProjection();
	s_Projection	 = camera.GetProjection();
	s_View			 = camera.GetViewMatrix();

	s_Data.MatricesBuffer->Bind();
	s_Data.MatricesBuffer->SetData(glm::value_ptr(s_Projection), sizeof(glm::mat4));
	s_Data.MatricesBuffer->SetData(glm::value_ptr(s_View), sizeof(glm::mat4), sizeof(glm::mat4));

	s_Data.CubeShader->Bind();
	s_Data.CubeShader->SetUniform3f("u_ViewPos", camera.Position);

	StartBatch();
}

void Renderer::SceneEnd()
{
	Flush();
}

void Renderer::Flush()
{
	if (s_Data.QuadIndexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadBufferPtr - (uint8_t*)s_Data.QuadBufferBase);
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadBufferBase, dataSize);

		GLCall(glDisable(GL_CULL_FACE));
		DrawIndexed(s_Data.QuadShader, s_Data.QuadVertexArray, s_Data.QuadIndexCount, GL_TRIANGLES);
		GLCall(glEnable(GL_CULL_FACE));
	}

	if (s_Data.LineVertexCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineBufferPtr - (uint8_t*)s_Data.LineBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineBufferBase, dataSize);

		GLCall(glDisable(GL_CULL_FACE));
		DrawArrays(s_Data.LineShader, s_Data.LineVertexArray, s_Data.LineVertexCount, GL_LINES);
		GLCall(glEnable(GL_CULL_FACE));
	}

	if (s_Data.CubeInstanceCount)
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeBufferPtr - (uint8_t*)s_Data.CubeBufferBase);
		s_Data.CubeInstanceBuffer->SetData(s_Data.CubeBufferBase, dataSize);

		s_Data.CubeShader->Bind();
		s_Data.CubeShader->SetUniform1i("u_DirLightsCount", s_Data.DirLightsCount);
		s_Data.CubeShader->SetUniform1i("u_PointLightsCount", s_Data.PointLightsCount);
		s_Data.CubeShader->SetUniform1i("u_SpotLightsCount", s_Data.SpotLightsCount);

		for (int32_t i = 0; i < s_Data.BoundTexturesCount; i++)
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + i));
			GLCall(glBindTexture(GL_TEXTURE_2D, s_Data.TextureBindings[i]));
		}

		DrawArraysInstanced(s_Data.CubeShader, s_Data.CubeVertexArray, s_Data.CubeInstanceCount);
	}
}

void Renderer::ClearColor(const glm::vec4& color)
{
	GLCall(glClearColor(color.r, color.g, color.b, color.a));
}

void Renderer::Clear()
{
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

void Renderer::DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
{
	if (s_Data.QuadIndexCount + 6 > s_Data.MaxIndices)
	{
		NextBatch();
	}

	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);

	for (size_t i = 0; i < s_Data.QuadVertices.size(); ++i)
	{
		s_Data.QuadBufferPtr->Position = transform * s_Data.QuadVertices[i];
		s_Data.QuadBufferPtr->Color = color;
					   
		++s_Data.QuadBufferPtr;
	}

	s_Data.QuadIndexCount += 6;
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

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
{
	if (s_Data.CubeInstanceCount >= 254)
	{
		NextBatch();
	}

	s_Data.CubeBufferPtr->Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
	s_Data.CubeBufferPtr->Color = color;
	s_Data.CubeBufferPtr->TextureSlot = -1.0f;

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

void Renderer::DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t count, uint32_t primitiveType)
{
	uint32_t indices = count ? count : vao->GetIndexBuffer()->GetCount();
	vao->Bind();
	shader->Bind();

	GLCall(glDrawElements(primitiveType, indices, GL_UNSIGNED_INT, nullptr));
}

void Renderer::DrawArrays(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t vertexCount, uint32_t primitiveType)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawArrays(primitiveType, 0, vertexCount));
}

void Renderer::DrawArraysInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances)
{
	vao->Bind();
	shader->Bind();

	GLCall(glDrawArraysInstanced(GL_TRIANGLES, 0, vao->VertexCount(), instances));
}

void Renderer::AddDirectionalLight(const DirectionalLight& light)
{
	s_Data.CubeShader->Bind();
	s_Data.CubeShader->SetUniform3f("u_DirLights[" + std::to_string(s_Data.DirLightsCount) + "].direction", light.Direction);
	s_Data.CubeShader->SetUniform3f("u_DirLights[" + std::to_string(s_Data.DirLightsCount) + "].color",		light.Color);

	s_Data.DirLightsCount++;
}

void Renderer::AddPointLight(const PointLight& light)
{
	s_Data.CubeShader->Bind();
	s_Data.CubeShader->SetUniform3f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].position",		light.Position);
	s_Data.CubeShader->SetUniform3f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].color",			light.Color);
	s_Data.CubeShader->SetUniform1f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].linearTerm",	light.LinearTerm);
	s_Data.CubeShader->SetUniform1f("u_PointLights[" + std::to_string(s_Data.PointLightsCount) + "].quadraticTerm", light.QuadraticTerm);

	s_Data.PointLightsCount++;
}

void Renderer::AddSpotLight(const SpotLight& light)
{
	s_Data.CubeShader->Bind();
	s_Data.CubeShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].position",	   light.Position);
	s_Data.CubeShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].cutOff",		   glm::cos(glm::radians(light.CutOff)));
	s_Data.CubeShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].direction",	   light.Direction);
	s_Data.CubeShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].outerCutOff",   glm::cos(glm::radians(light.OuterCutOff)));
	s_Data.CubeShader->SetUniform3f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].color",		   light.Color);
	s_Data.CubeShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].linearTerm",	   light.LinearTerm);
	s_Data.CubeShader->SetUniform1f("u_SpotLights[" + std::to_string(s_Data.PointLightsCount) + "].quadraticTerm", light.QuadraticTerm);

	s_Data.SpotLightsCount++;
}

void Renderer::StartBatch()
{
	s_Data.QuadIndexCount = 0;
	s_Data.QuadBufferPtr = s_Data.QuadBufferBase;

	s_Data.LineVertexCount = 0;
	s_Data.LineBufferPtr = s_Data.LineBufferBase;

	s_Data.CubeInstanceCount = 0;
	s_Data.CubeBufferPtr = s_Data.CubeBufferBase;

	s_Data.DirLightsCount = 0;
	s_Data.PointLightsCount = 0;
	s_Data.SpotLightsCount = 0;

	s_Data.BoundTexturesCount = 0;
}

void Renderer::NextBatch()
{
	Flush();
	StartBatch();
}
