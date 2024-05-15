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

Shader::Shader(const ShaderSpec& spec)
	: m_Spec(spec)
{
	std::optional<std::string> vertex = ParseShaderSource(spec.Vertex.Path);
	if (!vertex.has_value())
	{
		LOG_ERROR("Failed to open vertex shader file: {}", spec.Vertex.Path);

		return;
	}

	std::optional<std::string> fragment = ParseShaderSource(spec.Fragment.Path);
	if (!fragment.has_value())
	{
		LOG_ERROR("Failed to open fragment shader file: {}", spec.Fragment.Path);

		return;
	}

	std::optional<std::string> geometry{};
	if (spec.Geometry.has_value())
	{
		geometry = ParseShaderSource(spec.Geometry.value().Path);
		if (!geometry.has_value())
		{
			LOG_ERROR("Failed to open geometry shader file: {}", spec.Geometry.value().Path);

			return;
		}
	}

	m_ID = CreateShader(vertex.value(), fragment.value(), geometry);
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

void Shader::Reload()
{
	m_UniformLocations.clear();

	if (m_ID != 0)
	{
		Unbind();
		GLCall(glDeleteProgram(m_ID));
	}

	std::optional<std::string> vertex = ParseShaderSource(m_Spec.Vertex.Path);
	if (!vertex.has_value())
	{
		LOG_ERROR("Failed to open vertex shader file: {}", m_Spec.Vertex.Path);

		return;
	}

	std::optional<std::string> fragment = ParseShaderSource(m_Spec.Fragment.Path);
	if (!fragment.has_value())
	{
		LOG_ERROR("Failed to open fragment shader file: {}", m_Spec.Fragment.Path);

		return;
	}

	std::optional<std::string> geometry{};
	if (m_Spec.Geometry.has_value())
	{
		geometry = ParseShaderSource(m_Spec.Geometry.value().Path);
		if (!geometry.has_value())
		{
			LOG_ERROR("Failed to open geometry shader file: {}", m_Spec.Geometry.value().Path);

			return;
		}
	}

	m_ID = CreateShader(vertex.value(), fragment.value(), geometry);
}

void Shader::SetUniform1i(const std::string& name, int32_t val)
{
	GLCall(glUniform1i(UniformLocation(name), val));
}

void Shader::SetUniform1f(const std::string& name, float val)
{
	GLCall(glUniform1f(UniformLocation(name), val));
}

void Shader::SetUniform2f(const std::string& name, const glm::vec2& vec)
{
	GLCall(glUniform2f(UniformLocation(name), vec.r, vec.g));
}

void Shader::SetUniform3f(const std::string& name, const glm::vec3& vec)
{
	GLCall(glUniform3f(UniformLocation(name), vec.r, vec.g, vec.b));
}

void Shader::SetUniform4f(const std::string& name, const glm::vec4& vec)
{
	GLCall(glUniform4f(UniformLocation(name), vec.r, vec.g, vec.b, vec.a));
}

void Shader::SetUniformMat4(const std::string& name, const glm::mat4& vec)
{
	GLCall(glUniformMatrix4fv(UniformLocation(name), 1, GL_FALSE, &vec[0][0]));
}

void Shader::SetUniformBool(const std::string& name, bool flag)
{
	GLCall(glUniform1i(UniformLocation(name), flag ? 1 : 0));
}

std::optional<std::string> Shader::ParseShaderSource(const std::string& path)
{
	std::ifstream shaderSourceFile(path);
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

uint32_t Shader::CreateShader(const std::string& vsrc, const std::string& fsrc, std::optional<std::string> gsrc)
{
	GLCall(uint32_t program = glCreateProgram());
	uint32_t vsID = CompileShader(GL_VERTEX_SHADER, vsrc);
	uint32_t fsID = CompileShader(GL_FRAGMENT_SHADER, fsrc);
	uint32_t gsID = 0;

	if (gsrc.has_value())
	{
		GLCall(gsID = CompileShader(GL_GEOMETRY_SHADER, gsrc.value()));
	}

	GLCall(glAttachShader(program, vsID));
	GLCall(glAttachShader(program, fsID));

	if (gsID != 0)
	{
		GLCall(glAttachShader(program, gsID));
	}

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

	if (gsID != 0)
	{
		GLCall(glDeleteShader(gsID));
	}

	return program;
}

uint32_t Shader::CompileShader(uint32_t type, const std::string& source)
{
	GLCall(uint32_t id = glCreateShader(type));
	const char* src = source.c_str();
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

int32_t Shader::UniformLocation(const std::string& name)
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

struct RenderbufferSettings
{
	int32_t Type = 0;
	int32_t AttachmentType = 0;
};

static RenderbufferSettings RBO_Settings(const RenderbufferSpec& spec)
{
	RenderbufferSettings sets{};

	switch (spec.Type)
	{
	case RenderbufferType::DEPTH:
		sets.Type = GL_DEPTH_COMPONENT;
		sets.AttachmentType = GL_DEPTH_ATTACHMENT;
		break;
	case RenderbufferType::STENCIL:
		sets.Type = GL_STENCIL_COMPONENTS;
		sets.AttachmentType = GL_STENCIL_ATTACHMENT;
		break;
	case RenderbufferType::DEPTH_STENCIL:
		sets.Type = GL_DEPTH24_STENCIL8;
		sets.AttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
		break;
	default:
		assert(true && "Invalid renderbuffer type passed.");
		break;
	}

	return sets;
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
	case TextureFormat::RGB32F:
		formatInfo.InternalFormat = GL_RGB32F;
		formatInfo.Format = GL_RGB;
		formatInfo.Type = GL_FLOAT;
		formatInfo.BPP = 3;
		break;
	case TextureFormat::R11_G11_B10:
		formatInfo.InternalFormat = GL_R11F_G11F_B10F;
		formatInfo.Format = GL_RGB;
		formatInfo.Type = GL_FLOAT;
		formatInfo.BPP = 3;
		break;
	case TextureFormat::DEPTH_32F:
		formatInfo.InternalFormat = GL_DEPTH_COMPONENT32F;
		formatInfo.Format = GL_DEPTH_COMPONENT;
		formatInfo.Type = GL_FLOAT;
		formatInfo.BPP = 1;
		break;
	default:
		assert(true && "Invalid texture format passed");
		break;
	}

	return formatInfo;
}

static GLenum TexType(const ColorAttachmentType& type)
{
	GLenum ret{};

	switch (type)
	{
	case ColorAttachmentType::TEX_2D:
		ret = GL_TEXTURE_2D;
		break;
	case ColorAttachmentType::TEX_2D_MULTISAMPLE:
		ret = GL_TEXTURE_2D_MULTISAMPLE;
		break;
	case ColorAttachmentType::TEX_2D_ARRAY:
	case ColorAttachmentType::TEX_2D_ARRAY_SHADOW:
		ret = GL_TEXTURE_2D_ARRAY;
		break;
	case ColorAttachmentType::TEX_CUBEMAP:
		ret = GL_TEXTURE_CUBE_MAP;
		break;
		break;
	case ColorAttachmentType::TEX_CUBEMAP_ARRAY:
		ret = GL_TEXTURE_CUBE_MAP_ARRAY;
		break;
	default:
		assert(true && "Unsupported texture type passed");
		break;
	}

	return ret;
}

Framebuffer::Framebuffer(uint32_t samples)
	: m_Samples(samples)
{
	assert(samples > 0 && "At least 1 sample required.");

	GLCall(glGenFramebuffers(1, &m_ID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));
}

Framebuffer::~Framebuffer()
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	if (m_ID != 0)
	{
		GLCall(glDeleteFramebuffers(1, &m_ID));
	}

	if (m_RenderbufferID != 0)
	{
		GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
		GLCall(glDeleteRenderbuffers(1, &m_RenderbufferID));
	}

	for (const auto& [id, spec] : m_ColorAttachments)
	{
		GLCall(glBindTexture(TexType(spec.Type), 0));
		GLCall(glDeleteTextures(1, &id));
	}
}

void Framebuffer::Bind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));
}

void Framebuffer::Unbind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Framebuffer::BlitBuffers(uint32_t sourceAttachment, uint32_t targetAttachment, const Framebuffer& target) const
{
	assert(m_ID != 0 && target.m_ID != 0 && "One of the framebuffers is empty.");

	GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID));
	GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.m_ID));
	GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0 + sourceAttachment));
	GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0 + targetAttachment));
	GLCall(glBlitFramebuffer(0, 0, m_RBO_Spec.Size.x, m_RBO_Spec.Size.y, 0, 0, m_RBO_Spec.Size.x, m_RBO_Spec.Size.y, GL_COLOR_BUFFER_BIT, GL_LINEAR));
}

void Framebuffer::ResizeRenderbuffer(const glm::uvec2& size)
{
	bool multisampled = m_Samples > 1;
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));

	if (multisampled)
	{
		GLCall(glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, RBO_Settings(m_RBO_Spec).Type, size.x, size.y));
	}
	else
	{
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, RBO_Settings(m_RBO_Spec).Type, size.x, size.y));
	}

	m_RBO_Spec.Size = size;
}

void Framebuffer::ResizeEverything(const glm::uvec2& size)
{
	bool multisampled = m_Samples > 1;
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));

	if (multisampled)
	{
		for (auto& [id, spec] : m_ColorAttachments)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id));
			GLCall(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Samples, FormatInfo(spec.Format).InternalFormat, size.x, size.y, GL_TRUE));
			spec.Size = size;
		}

		GLCall(glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, RBO_Settings(m_RBO_Spec).Type, size.x, size.y));
	}
	else
	{
		for (auto& [id, spec] : m_ColorAttachments)
		{
			TexFormatInfo texFmt = FormatInfo(spec.Format);
			GLenum texType = TexType(spec.Type);
			GLCall(glBindTexture(texType, id));

			if (spec.Type == ColorAttachmentType::TEX_CUBEMAP)
			{
				for (uint32_t i = 0; i < 6; i++)
				{
					GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, texFmt.InternalFormat, spec.Size.x, spec.Size.y, 0, texFmt.Format, texFmt.Type, nullptr));
				}
			}
			else
			{
				GLCall(glTexImage2D(texType, 0, texFmt.InternalFormat, size.x, size.y, 0, texFmt.Format, texFmt.Type, nullptr));
			}

			spec.Size = size;
		}

		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, RBO_Settings(m_RBO_Spec).Type, size.x, size.y));
	}

	m_RBO_Spec.Size = size;
}

void Framebuffer::FillDrawBuffers()
{
	std::vector<GLenum> buffers(m_ColorAttachments.size());
	for (size_t i = 0; i < m_ColorAttachments.size(); i++)
	{
		buffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	GLCall(glDrawBuffers(buffers.size(), buffers.data()));
}

void Framebuffer::AddRenderbuffer(RenderbufferSpec spec)
{
	assert(m_ID > 0 && "Cannot create renderbuffer for empty framebuffer.");

	GLCall(glGenRenderbuffers(1, &m_RenderbufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));

	RenderbufferSettings sets = RBO_Settings(spec);
	if (m_Samples > 1)
	{
		GLCall(glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, sets.Type, spec.Size.x, spec.Size.y));
	}
	else
	{
		GLCall(glRenderbufferStorage(GL_RENDERBUFFER, sets.Type, spec.Size.x, spec.Size.y));
	}

	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, sets.AttachmentType, GL_RENDERBUFFER, m_RenderbufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	m_RBO_Spec = spec;
}

void Framebuffer::AddColorAttachment(ColorAttachmentSpec spec)
{
	assert(m_ID > 0 && "Cannot create color attachment for empty framebuffer.");

	uint32_t id{};
	GLCall(glGenTextures(1, &id));
	TexFormatInfo texFmt = FormatInfo(spec.Format);

	if (m_Samples > 1)
	{
		assert(spec.Type == ColorAttachmentType::TEX_2D_MULTISAMPLE && "Only multisampled texture supported for multisampled framebuffer.");

		GLCall(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id));
		GLCall(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Samples, texFmt.InternalFormat, spec.Size.x, spec.Size.y, GL_TRUE));
	}
	else
	{
		assert(spec.Type != ColorAttachmentType::TEX_2D_MULTISAMPLE && "Only multisampled texture supported for multisampled framebuffer.");
		int32_t type = TexType(spec.Type);

		GLCall(glBindTexture(type, id));
		GLCall(glTexParameteri(type, GL_TEXTURE_MIN_FILTER, spec.MinFilter));
		GLCall(glTexParameteri(type, GL_TEXTURE_MAG_FILTER, spec.MagFilter));
		GLCall(glTexParameteri(type, GL_TEXTURE_WRAP_S, spec.Wrap));
		GLCall(glTexParameteri(type, GL_TEXTURE_WRAP_T, spec.Wrap));

		switch (type)
		{
		case GL_TEXTURE_2D:
			GLCall(glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, &spec.BorderColor[0]));
			GLCall(glTexImage2D(type, 0, texFmt.InternalFormat, spec.Size.x, spec.Size.y, 0, texFmt.Format, texFmt.Type, nullptr));
			break;
		case GL_TEXTURE_2D_ARRAY:
			if (spec.Type == ColorAttachmentType::TEX_2D_ARRAY_SHADOW)
			{
				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
			}

			GLCall(glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, &spec.BorderColor[0]));
			GLCall(glTexImage3D(type, 0, texFmt.InternalFormat, spec.Size.x, spec.Size.y, 16, 0, texFmt.Format, texFmt.Type, nullptr));
			break;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			GLCall(glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, &spec.BorderColor[0]));
			GLCall(glTexStorage3D(type, 1, texFmt.InternalFormat, spec.Size.x, spec.Size.y, 6 * 16));
			break;
		case GL_TEXTURE_CUBE_MAP:
			GLCall(glTexParameteri(type, GL_TEXTURE_WRAP_R, spec.Wrap));
			for (uint32_t i = 0; i < 6; i++)
			{
				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, texFmt.InternalFormat, spec.Size.x, spec.Size.y, 0, texFmt.Format, texFmt.Type, nullptr));
			}
			break;
		default:
			assert(true && "Unsupported texture type.");
			break;
		}

		if (spec.GenMipmaps)
		{
			GLCall(glGenerateMipmap(type));
		}
	}

	m_ColorAttachments.push_back({ id, spec });
}

void Framebuffer::BindRenderbuffer() const
{
	assert(m_RenderbufferID != 0 && "Trying to bind non-existent renderbuffer.");

	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_RenderbufferID));
	GLCall(glViewport(0, 0, m_RBO_Spec.Size.x, m_RBO_Spec.Size.y));
}

void Framebuffer::BindColorAttachment(uint32_t attachmentIdx, uint32_t slot) const
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to bind non-existent color attachment.");

	const auto& [id, spec] = m_ColorAttachments[attachmentIdx];
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(TexType(spec.Type), id));
}

void Framebuffer::DrawToColorAttachment(uint32_t attachmentIdx, uint32_t targetAttachment, int32_t mip) const
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to draw to non-existent color attachment.");

	const auto& [id, spec] = m_ColorAttachments[attachmentIdx];
	Bind();
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + targetAttachment, TexType(spec.Type), id, mip));
}

void Framebuffer::DrawToDepthMap(uint32_t attachmentIdx, int32_t mip) const
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to draw to non-existent color attachment.");

	const auto& [id, spec] = m_ColorAttachments[attachmentIdx];
	Bind();
	GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, id, 0));
	GLCall(glViewport(0, 0, spec.Size.x, spec.Size.y));
}

void Framebuffer::DrawToCubeColorAttachment(uint32_t attachmentIdx, uint32_t targetAttachment, int32_t faceIdx, int32_t mip) const
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to draw to non-existent color attachment.");

	const auto& [id, spec] = m_ColorAttachments[attachmentIdx];
	Bind();
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + targetAttachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, id, mip));
}

void Framebuffer::ClearColorAttachment(uint32_t attachmentIdx, uint32_t mip) const
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to clear non-existent color attachment.");

	const auto& [id, spec] = m_ColorAttachments[attachmentIdx];
	TexFormatInfo texFmt = FormatInfo(spec.Format);
	float clear = -1.0f;
	GLCall(glClearTexImage(m_ColorAttachments[attachmentIdx].ID, mip, texFmt.Format, GL_FLOAT, &clear));
}

void Framebuffer::RemoveRenderbuffer()
{
	assert(m_RenderbufferID != 0 && "Trying to remove non-existent renderbuffer");

	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
	GLCall(glDeleteRenderbuffers(1, &m_RenderbufferID));
	m_RBO_Spec = {};
}

void Framebuffer::RemoveColorAttachment(uint32_t attachmentIdx)
{
	assert(attachmentIdx < m_ColorAttachments.size() && "Trying to remove non-existent color attachment.");

	GLenum texType = TexType(m_ColorAttachments[attachmentIdx].spec.Type);
	GLCall(glBindTexture(texType, 0));
	GLCall(glDeleteTextures(1, &m_ColorAttachments[attachmentIdx].ID));
	
	m_ColorAttachments.erase(m_ColorAttachments.begin() + attachmentIdx);
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
	GLCall(return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

Texture::Texture(const std::string& path, TextureFormat format)
	: m_ID(0), m_Width(0), m_Height(0), m_BPP(0), m_Path(path)
{
	auto [internalFormat, pixelFormat, type, BPP] = FormatInfo(format);
	void* buffer = nullptr;
	stbi_set_flip_vertically_on_load(1);
	if (type == GL_FLOAT)
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
