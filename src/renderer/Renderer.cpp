#include "Renderer.hpp"
#include "OpenGL.hpp"
#include "../Logger.hpp"
#include "../Timer.hpp"

#include <glm/gtc/matrix_transform.hpp>

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

struct RendererData
{
	static constexpr uint32_t MaxQuads = 5000;
	static constexpr uint32_t MaxVertices = MaxQuads * 4;
	static constexpr uint32_t MaxIndices = MaxQuads * 6;

	std::shared_ptr<VertexArray>  QuadVertexArray;
	std::shared_ptr<VertexBuffer> QuadVertexBuffer;
	std::shared_ptr<Shader>		  QuadShader;

	uint32_t	QuadIndexCount = 0;
	QuadVertex* QuadBufferBase = nullptr;
	QuadVertex* QuadBufferPtr = nullptr;

	std::array<glm::vec4, 4> QuadVertices;
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
}

void Renderer::Shutdown()
{
	delete[] s_Data.QuadBufferBase;
}

void Renderer::ReloadShaders()
{
	s_Data.QuadShader->ReloadShader();
}

void Renderer::OnWindowResize(const Viewport& newViewport)
{
	auto& [x, y, width, height] = newViewport;
	GLCall(glViewport(x, y, width, height));
}

void Renderer::SceneBegin()
{
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
		DrawIndexed(s_Data.QuadShader, s_Data.QuadVertexArray, s_Data.QuadIndexCount);
		GLCall(glEnable(GL_CULL_FACE));
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

void Renderer::DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t count)
{
	uint32_t indices = count ? count : vao->GetIndexBuffer()->GetCount();

	shader->Bind();
	shader->SetUniformMat4("u_ViewProjection", s_ViewProjection);

	vao->Bind();

	GLCall(glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, nullptr));
}

void Renderer::StartBatch()
{
	s_Data.QuadIndexCount = 0;
	s_Data.QuadBufferPtr = s_Data.QuadBufferBase;
}

void Renderer::NextBatch()
{
	Flush();
	StartBatch();
}
