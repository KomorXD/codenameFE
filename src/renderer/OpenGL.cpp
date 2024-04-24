#include "OpenGL.hpp"
#include "../Logger.hpp"

#include <fstream>
#include <filesystem>
#include "stb/stb_image.h"

void GLClearErrors()
{
	while (glGetError() != GL_NO_ERROR);
}

bool GLCheckForErrors(const char* func, const char* filename, int32_t line)
{
	if (GLenum error = glGetError())
	{
		LOG_WARN("[OpenGL error {}]: {} at {} while calling {}", error, filename, line, func);

		return false;
	}

	return true;
}

VertexBuffer::VertexBuffer(const void* data, uint64_t size, uint32_t count)
	: m_VertexCount(count)
{
	GLCall(glGenBuffers(1, &m_ID));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

VertexBuffer::~VertexBuffer()
{
	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteBuffers(1, &m_ID));
	}
}

void VertexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
}

void VertexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset) const
{
	Bind();
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, size, data));
}

IndexBuffer::IndexBuffer(const uint32_t* data, uint32_t count)
	: m_Count(count)
{
	GLCall(glGenBuffers(1, &m_ID));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), data, GL_DYNAMIC_DRAW));
}

IndexBuffer::~IndexBuffer()
{
	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteBuffers(1, &m_ID));
	}
}

void IndexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID));
}

void IndexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

uint32_t VertexBufferElement::GetSizeOfType(uint32_t type)
{
	switch (type)
	{
	case GL_FLOAT:			return sizeof(GLfloat);
	case GL_UNSIGNED_INT:	return sizeof(GLuint);
	case GL_UNSIGNED_BYTE:	return sizeof(GLbyte);
	}

	return 0;
}

VertexArray::VertexArray()
{
	GLCall(glGenVertexArrays(1, &m_ID));
	GLCall(glBindVertexArray(m_ID));
}

VertexArray::~VertexArray()
{
	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteVertexArrays(1, &m_ID));
	}
}

void VertexArray::AddBuffers(const std::shared_ptr<VertexBuffer>& vbo, std::unique_ptr<IndexBuffer>& ibo, const VertexBufferLayout& layout, uint32_t attribOffset)
{
	Bind();
	vbo->Bind();
	ibo->Bind();

	const auto& elements = layout.GetElements();
	uint32_t offset = 0;

	for (uint32_t i = attribOffset; i < elements.size() + attribOffset; ++i)
	{
		const VertexBufferElement& element = elements[i - attribOffset];

		GLCall(glEnableVertexAttribArray(i));
		GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));

		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
	}

	m_VertexCount = vbo->VertexCount();
	m_IBO = std::move(ibo);
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vbo, const VertexBufferLayout& layout, uint32_t attribOffset)
{
	Bind();
	vbo->Bind();

	const auto& elements = layout.GetElements();
	uint32_t offset = 0;

	for (uint32_t i = attribOffset; i < elements.size() + attribOffset; ++i)
	{
		const VertexBufferElement& element = elements[i - attribOffset];

		GLCall(glEnableVertexAttribArray(i));
		GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));

		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
	}
	
	m_VertexCount = vbo->VertexCount();
}

void VertexArray::AddInstancedVertexBuffer(const std::shared_ptr<VertexBuffer>& vbo, const VertexBufferLayout& layout, uint32_t attribOffset) const
{
	Bind();
	vbo->Bind();

	const auto& elements = layout.GetElements();
	uint32_t offset = 0;

	for (uint32_t i = attribOffset; i < elements.size() + attribOffset; ++i)
	{
		const VertexBufferElement& element = elements[i - attribOffset];

		GLCall(glEnableVertexAttribArray(i));
		GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));
		GLCall(glVertexAttribDivisor(i, 1));

		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
	}
}

void VertexArray::Bind() const
{
	GLCall(glBindVertexArray(m_ID));
}

void VertexArray::Unbind() const
{
	GLCall(glBindVertexArray(0));
}

UniformBuffer::UniformBuffer(const void* data, uint64_t size)
{
	GLCall(glGenBuffers(1, &m_ID));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_ID));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

UniformBuffer::~UniformBuffer()
{
	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteBuffers(1, &m_ID));
	}
}

void UniformBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_ID));
}

void UniformBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UniformBuffer::BindBufferRange(uint32_t bufferIndex, uint32_t offset, uint32_t size)
{
	GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, bufferIndex, m_ID, (GLintptr)offset, size));
}

void UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) const
{
	Bind();
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, size, data));
}

Shader::Shader(const std::string& vs, const std::string& fs)
	: m_VertexShaderPath(vs), m_FragmentShaderPath(fs), m_ID(0)
{
	std::optional<std::string> vertexSource = ShaderParse(vs);

	if (!vertexSource.has_value())
	{
		LOG_ERROR("Failed to open vertex shader file: {}", vs);

		return;
	}

	std::optional<std::string> fragmentSource = ShaderParse(fs);

	if (!fragmentSource.has_value())
	{
		LOG_ERROR("Failed to open fragment shader file: {}", fs);

		return;
	}

	m_ID = ShaderCreate(vertexSource.value(), fragmentSource.value());
}

Shader::Shader(const std::string& vs, const std::string& gs, const std::string& fs)
	: m_VertexShaderPath(vs), m_GeometryShaderPath(gs), m_FragmentShaderPath(fs), m_ID(0)
{
	std::optional<std::string> vertexSource = ShaderParse(vs);

	if (!vertexSource.has_value())
	{
		LOG_ERROR("Failed to open vertex shader file: {}", vs);

		return;
	}

	std::optional<std::string> geometrySource = ShaderParse(gs);

	if (!geometrySource.has_value())
	{
		LOG_ERROR("Failed to open geometry shader file: {}", gs);

		return;
	}

	std::optional<std::string> fragmentSource = ShaderParse(fs);

	if (!fragmentSource.has_value())
	{
		LOG_ERROR("Failed to open fragment shader file: {}", fs);

		return;
	}

	m_ID = ShaderCreate(vertexSource.value(), geometrySource.value(), fragmentSource.value());
}

Shader::~Shader()
{
	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteProgram(m_ID));
	}
}

void Shader::Bind() const
{
	GLCall(glUseProgram(m_ID));
}

void Shader::Unbind() const
{
	GLCall(glUseProgram(0));
}

void Shader::SetUniform1i(const std::string& name, int32_t val)
{
	GLCall(glUniform1i(GetUniformLocation(name), val));
}

void Shader::SetUniform1f(const std::string& name, float val)
{
	GLCall(glUniform1f(GetUniformLocation(name), val));
}

void Shader::SetUniform3f(const std::string& name, const glm::vec3& vec)
{
	GLCall(glUniform3f(GetUniformLocation(name), vec.r, vec.g, vec.b));
}

void Shader::SetUniform4f(const std::string& name, const glm::vec4& vec)
{
	GLCall(glUniform4f(GetUniformLocation(name), vec.r, vec.g, vec.b, vec.a));
}

void Shader::SetUniformMat4(const std::string& name, const glm::mat4& vec)
{
	GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &vec[0][0]));
}

void Shader::SetUniformBool(const std::string& name, bool flag)
{
	GLCall(glUniform1i(GetUniformLocation(name), flag ? 1 : 0));
}

bool Shader::ReloadShader()
{
	Unbind();
	GLCall(glDeleteProgram(m_ID));

	std::optional<std::string> vertexSource = ShaderParse(m_VertexShaderPath);

	if (!vertexSource.has_value())
	{
		LOG_ERROR("Failed to open vertex shader file: {}", m_VertexShaderPath);

		return false;
	}

	std::optional<std::string> fragmentSource = ShaderParse(m_FragmentShaderPath);

	if (!fragmentSource.has_value())
	{
		LOG_ERROR("Failed to open fragment shader file: {}", m_FragmentShaderPath);

		return false;
	}

	if (m_GeometryShaderPath.empty())
	{
		m_ID = ShaderCreate(vertexSource.value(), fragmentSource.value());

		return m_ID != 0;
	}

	std::optional<std::string> geometrySource = ShaderParse(m_GeometryShaderPath);

	if (!geometrySource.has_value())
	{
		LOG_ERROR("Failed to open geometry shader file: {}", m_GeometryShaderPath);

		return false;
	}

	m_ID = ShaderCreate(vertexSource.value(), geometrySource.value(), fragmentSource.value());

	return m_ID != 0;
}

std::optional<std::string> Shader::ShaderParse(const std::string& filepath)
{
	std::ifstream shaderSourceFile(filepath);

	if (!shaderSourceFile.good())
	{
		return {};
	}

	std::string line{};
	std::stringstream ss{};

	while (std::getline(shaderSourceFile, line))
	{
		ss << line;
		ss << '\n';
	}

	return ss.str();
}

uint32_t Shader::ShaderCompile(uint32_t type, const std::string& sourceCode)
{
	GLCall(uint32_t id = glCreateShader(type));
	const char* src = sourceCode.c_str();
	int result = 0;

	GLCall(glShaderSource(id, 1, &src, nullptr));
	GLCall(glCompileShader(id));
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));

	if (result == GL_FALSE)
	{
		int len = 0;

		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len));

		char* message = (char*)_malloca(len * sizeof(char));

		GLCall(glGetShaderInfoLog(id, len, &len, message));

		LOG_ERROR("Failed to compile shader: {}", message);

		GLCall(glDeleteShader(id));

		return 0;
	}

	return id;
}

uint32_t Shader::ShaderCreate(const std::string& vs, const std::string& fs)
{
	GLCall(uint32_t program = glCreateProgram());
	uint32_t vsID = ShaderCompile(GL_VERTEX_SHADER, vs);
	uint32_t fsID = ShaderCompile(GL_FRAGMENT_SHADER, fs);

	GLCall(glAttachShader(program, vsID));
	GLCall(glAttachShader(program, fsID));
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));

	int success = 0;

	GLCall(glGetProgramiv(program, GL_LINK_STATUS, &success));

	if (success == GL_FALSE)
	{
		int len = 0;

		GLCall(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len));

		char* message = (char*)_malloca(len * sizeof(char));

		GLCall(glGetProgramInfoLog(program, len, &len, message));

		LOG_ERROR("Failed to link shaders: {}", message);

		GLCall(glDeleteProgram(program));

		return 0;
	}

	GLCall(glDeleteShader(vsID));
	GLCall(glDeleteShader(fsID));

	return program;
}

uint32_t Shader::ShaderCreate(const std::string& vs, const std::string& gs, const std::string& fs)
{
	GLCall(uint32_t program = glCreateProgram());
	uint32_t vsID = ShaderCompile(GL_VERTEX_SHADER, vs);
	uint32_t gsID = ShaderCompile(GL_GEOMETRY_SHADER, gs);
	uint32_t fsID = ShaderCompile(GL_FRAGMENT_SHADER, fs);

	GLCall(glAttachShader(program, vsID));
	GLCall(glAttachShader(program, gsID));
	GLCall(glAttachShader(program, fsID));
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));

	int success = 0;

	GLCall(glGetProgramiv(program, GL_LINK_STATUS, &success));

	if (success == GL_FALSE)
	{
		int len = 0;

		GLCall(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len));

		char* message = (char*)_malloca(len * sizeof(char));

		GLCall(glGetProgramInfoLog(program, len, &len, message));

		LOG_ERROR("Failed to link shaders: {}", message);

		GLCall(glDeleteProgram(program));

		return 0;
	}

	GLCall(glDeleteShader(vsID));
	GLCall(glDeleteShader(gsID));
	GLCall(glDeleteShader(fsID));

	return program;
}

int32_t Shader::GetUniformLocation(const std::string& name)
{
	if (m_UniformLocations.contains(name))
	{
		return m_UniformLocations[name];
	}

	GLCall(int32_t location = glGetUniformLocation(m_ID, name.c_str()));

	if (location == -1)
	{
		LOG_WARN("Uniform {} does not exist or is not in use", name);

		return -1;
	}

	m_UniformLocations[name] = location;

	return location;
}

SharedBuffer::SharedBuffer(const void* data, uint64_t size)
{
	GLCall(glGenBuffers(1, &m_ID));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID));
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

SharedBuffer::~SharedBuffer()
{
	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteBuffers(1, &m_ID));
	}
}

void SharedBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID));
}

void SharedBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}

void SharedBuffer::BindBufferSlot(uint32_t bufferIndex)
{
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bufferIndex, m_ID));
}

void SharedBuffer::ResetData(const void* data, uint64_t size) const
{
	Bind();
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

void SharedBuffer::SetData(const void* data, uint32_t size, uint32_t offset) const
{
	Bind();
	GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)offset, size, data));
}

Framebuffer::Framebuffer(const glm::uvec2& bufferSize, uint32_t samples)
	: m_Samples(samples), m_BufferSize(bufferSize)
{
	assert(samples > 0 && "Samples valie should be at least 1.");

	GLCall(glGenFramebuffers(1, &m_ID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));

	GLCall(glGenRenderbuffers(1, &m_RenderbufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));

	if (samples > 1)
	{
		GLCall(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, bufferSize.x, bufferSize.y));
	}
	else
	{
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufferSize.x, bufferSize.y));
	}

	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RenderbufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

Framebuffer::~Framebuffer()
{
	if (m_RenderbufferID != 0)
	{
		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
		GLCall(glDeleteRenderbuffers(1, &m_RenderbufferID));
	}
	
	for (auto& [id, format] : m_ColorAttachments)
	{
		GLCall(glDeleteTextures(1, &id));
	}
	
	if (m_ID != 0)
	{
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		GLCall(glDeleteFramebuffers(1, &m_ID));
	}
}

void Framebuffer::Bind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));
	GLCall(glViewport(0, 0, m_BufferSize.x, m_BufferSize.y));
}

void Framebuffer::Unbind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Framebuffer::Resize(const glm::uvec2& bufferSize)
{
	bool multisampled = m_Samples > 1;
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));

	if (multisampled)
	{
		for (auto& [id, format] : m_ColorAttachments)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id));
			GLCall(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Samples, format, bufferSize.x, bufferSize.y, GL_TRUE));
		}

		GLCall(glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, GL_DEPTH24_STENCIL8, bufferSize.x, bufferSize.y));
	}
	else
	{
		for (auto& [id, format] : m_ColorAttachments)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D, id));
			GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, bufferSize.x, bufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
		}

		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufferSize.x, bufferSize.y));
	}

	m_BufferSize = bufferSize;
}

void Framebuffer::BlitBuffers(uint32_t sourceAttachmentIndex, uint32_t targetAttachmentID, const Framebuffer& target) const
{
	GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID));
	GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.m_ID));
	GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0 + sourceAttachmentIndex));
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0 + targetAttachmentID));
	GLCall(glBlitFramebuffer(0, 0, m_BufferSize.x, m_BufferSize.y, 0, 0, target.m_BufferSize.x, target.m_BufferSize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST));
}

void Framebuffer::AddColorAttachment(GLenum format)
{
	assert(m_ColorAttachments.size() < 4 && "Framebuffer supports up to 4 color attachments");

	uint32_t id{};
	GLCall(glGenTextures(1, &id));

	if (m_Samples > 1)
	{
		GLCall(glGenTextures(1, &id));
		GLCall(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id));
		GLCall(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Samples, format, m_BufferSize.x, m_BufferSize.y, GL_TRUE));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_ColorAttachments.size(), GL_TEXTURE_2D_MULTISAMPLE, id, 0));
		GLCall(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0));
	}
	else
	{
		GLCall(glBindTexture(GL_TEXTURE_2D, id));
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, m_BufferSize.x, m_BufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_ColorAttachments.size(), GL_TEXTURE_2D, id, 0));
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	}

	m_ColorAttachments.push_back({ id, format });

	if (m_ColorAttachments.size() > 1)
	{
		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		GLCall(glDrawBuffers(m_ColorAttachments.size(), buffers));
	}
}

void Framebuffer::FillDrawBuffers()
{
	GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	GLCall(glDrawBuffers(m_ColorAttachments.size(), buffers));
}

void Framebuffer::BindColorAttachment(uint32_t slot) const
{
	assert(slot < m_ColorAttachments.size() && "Trying to access color attachment out of bounds.");

	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(m_Samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_ColorAttachments[slot].ID));
}

void Framebuffer::UnbindColorAttachment() const
{
	GLCall(glBindTexture(m_Samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, 0));
}

void Framebuffer::ClearColorAttachment(uint32_t attachmentIdx) const
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to access color attachment out of bounds.");
	
	float lol = -1.0f;
	GLCall(glClearTexImage(m_ColorAttachments[attachmentIdx].ID, 0, GL_RGBA, GL_FLOAT, &lol));
}

glm::u8vec4 Framebuffer::GetPixelAt(const glm::vec2& coords, int32_t attachmentIdx) const
{
	glm::u8vec4 pixel{};

	Bind();
	GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIdx));
	GLCall(glReadPixels((GLint)coords.x, (GLint)coords.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel[0]));

	return pixel;
}

bool Framebuffer::IsComplete() const
{
	GLCall(bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	return complete;
}

CubemapFramebuffer::CubemapFramebuffer(const glm::uvec2& bufferSize)
	: m_BufferSize(bufferSize)
{
	GLCall(glGenFramebuffers(1, &m_ID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));

	GLCall(glGenRenderbuffers(1, &m_RenderbufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));

	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, bufferSize.x, bufferSize.y));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RenderbufferID));

	// Cubemap
	GLCall(glGenTextures(1, &m_CubemapID));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapID));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	for (uint32_t i = 0; i < 6; i++)
	{
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, bufferSize.x, bufferSize.y, 0, GL_RGB, GL_FLOAT, nullptr));
	}

	// Irradiance map
	GLCall(glGenTextures(1, &m_IrradianceMapID));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMapID));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	for (uint32_t i = 0; i < 6; i++)
	{
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, bufferSize.x, bufferSize.y, 0, GL_RGB, GL_FLOAT, nullptr));
	}

	// Prefilter map
	GLCall(glGenTextures(1, &m_PrefilterID));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterID));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	for (uint32_t i = 0; i < 6; i++)
	{
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr));
	}
	GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

CubemapFramebuffer::~CubemapFramebuffer()
{
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	if (m_RenderbufferID != 0)
	{
		GLCall(glDeleteRenderbuffers(1, &m_RenderbufferID));
	}
	
	if(m_CubemapID != 0)
	{
		GLCall(glDeleteTextures(1, &m_CubemapID));
	}
	
	if (m_IrradianceMapID != 0)
	{
		GLCall(glDeleteTextures(1, &m_IrradianceMapID));
	}
	
	if(m_PrefilterID != 0)
	{
		GLCall(glDeleteTextures(1, &m_PrefilterID));
	}
	
	if (m_ID != 0)
	{
		GLCall(glDeleteFramebuffers(1, &m_ID));
	}
}

void CubemapFramebuffer::Bind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));
	GLCall(glViewport(0, 0, m_BufferSize.x, m_BufferSize.y));
}

void CubemapFramebuffer::Unbind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

void CubemapFramebuffer::BindCubemap(uint32_t slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapID));
}

void CubemapFramebuffer::BindIrradianceMap(uint32_t slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMapID));
}

void CubemapFramebuffer::BindPrefilterMap(uint32_t slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterID));
}

void CubemapFramebuffer::UnbindMaps() const
{
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void CubemapFramebuffer::ResizeRenderbuffer(const glm::uvec2& bufferSize)
{
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, bufferSize.x, bufferSize.y));
	GLCall(glViewport(0, 0, bufferSize.x, bufferSize.y));

	m_BufferSize = bufferSize;
}

void CubemapFramebuffer::SetCubemapFaceTarget(uint32_t faceIdx) const
{
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, m_CubemapID, 0));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, m_IrradianceMapID, 0));
}

void CubemapFramebuffer::SetPrefilterFaceTarget(uint32_t faceIdx, uint32_t mipmapLevel) const
{
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, m_PrefilterID, mipmapLevel));
}

static struct TexFormatInfo
{
	int32_t InternalFormat;
	int32_t Format;
	int32_t Type;
	int32_t BPP;
};

static TexFormatInfo FormatInfo(TextureFormat format)
{
	TexFormatInfo formatInfo{};

	switch (format)
	{
	case TextureFormat::RGBA8:
		formatInfo.InternalFormat = GL_RGBA8;
		formatInfo.Format = GL_RGBA;
		formatInfo.Type = GL_UNSIGNED_BYTE;
		formatInfo.BPP = 4;
		break;
	case TextureFormat::RGB8:
		formatInfo.InternalFormat = GL_RGB8;
		formatInfo.Format = GL_RGB;
		formatInfo.Type = GL_UNSIGNED_BYTE;
		formatInfo.BPP = 3;
		break;
	case TextureFormat::RGBA16F:
		formatInfo.InternalFormat = GL_RGBA16F;
		formatInfo.Format = GL_RGBA;
		formatInfo.Type = GL_FLOAT;
		formatInfo.BPP = 4;
		break;
	case TextureFormat::RGB16F:
		formatInfo.InternalFormat = GL_RGB16F;
		formatInfo.Format = GL_RGB;
		formatInfo.Type = GL_FLOAT;
		formatInfo.BPP = 3;
		break;
	case TextureFormat::RG16F:
		formatInfo.InternalFormat = GL_RG16F;
		formatInfo.Format = GL_RG;
		formatInfo.Type = GL_FLOAT;
		formatInfo.BPP = 2;
		break;
	default:
		ASSERT(true && "Invalid format enum");
		break;
	}

	return formatInfo;
}

Texture::Texture(const std::string& path, TextureFormat format)
	: m_ID(0), m_Width(0), m_Height(0), m_BPP(0), m_Path(path)
{
	auto [internalFormat, pixelFormat, type, BPP] = FormatInfo(format);
	void* buffer = nullptr;
	stbi_set_flip_vertically_on_load(1);
	if (format == TextureFormat::RGBA16F || format == TextureFormat::RGB16F)
	{
		buffer = stbi_loadf(path.c_str(), &m_Width, &m_Height, &m_BPP, BPP);
	}
	else
	{
		buffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, BPP);
	}

	GLCall(glGenTextures(1, &m_ID));
	GLCall(glBindTexture(GL_TEXTURE_2D, m_ID));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, pixelFormat, type, buffer));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	if (buffer)
	{
		stbi_image_free(buffer);
	}

	std::filesystem::path texturePath = path;
	m_Name = texturePath.filename().string();
}

Texture::Texture(const void* data, int32_t width, int32_t height, const std::string& name, TextureFormat format)
	: m_ID(0), m_Width(width), m_Height(height), m_BPP(0), m_Path(""), m_Name(name)
{
	GLCall(glGenTextures(1, &m_ID));
	GLCall(glBindTexture(GL_TEXTURE_2D, m_ID));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	auto [internalFormat, pixelFormat, type, BPP] = FormatInfo(format);
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, pixelFormat, type, data));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

Texture::Texture(uint32_t id, const std::string& name, TextureFormat format)
	: m_ID(id), m_Path(""), m_Name(name)
{
	ASSERT(id > 0 && "Invalid ID");

	GLCall(glBindTexture(GL_TEXTURE_2D, m_ID));
	GLCall(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_Width));
	GLCall(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_Height));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	m_BPP = FormatInfo(format).BPP;
}

Texture::~Texture()
{
	if (m_ID != 0)
	{
		GLCall(glDeleteTextures(1, &m_ID));
	}
}

void Texture::SetFilter(int32_t filter)
{
	int32_t mipmapFilter = filter == GL_LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;

	Bind();
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapFilter));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	m_Filter = filter;
}

void Texture::SetWrap(int32_t wrap)
{
	Bind();
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	m_Wrap = wrap;
}

void Texture::Bind(uint32_t slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, m_ID));
}

void Texture::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}
