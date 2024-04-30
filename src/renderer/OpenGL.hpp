#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>
#include <optional>
#include <memory>
#include <vector>

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#ifndef CONF_PROD
#ifdef TARGET_WINDOWS
#define ASSERT(x) if(!(x)) __debugbreak()
#else
#include <signal.h>
#define ASSERT(x) if(!(x)) raise(SIGTRAP)
#endif
#endif

#ifdef CONF_DEBUG
#define GLCall(f) GLClearErrors();\
	f;\
	ASSERT(GLCheckForErrors(#f, __FILENAME__, __LINE__))
#else
#define GLCall(f) f
#endif

void GLClearErrors();

bool GLCheckForErrors(const char* func, const char* filename, int32_t line);

class VertexBuffer
{
public:
	VertexBuffer(const void* data, uint64_t size, uint32_t count = 0);
	~VertexBuffer();

	void Bind() const;
	void Unbind() const;
	void SetData(const void* data, uint32_t size, uint32_t offset = 0) const;

	inline uint32_t VertexCount() const { return m_VertexCount; }

private:
	uint32_t m_ID = 0;
	uint32_t m_VertexCount = 0;
};

class IndexBuffer
{
public:
	IndexBuffer(const uint32_t* data, uint32_t count);
	~IndexBuffer();

	void Bind() const;
	void Unbind() const;

	inline uint32_t GetCount() const { return m_Count; }

private:
	uint32_t m_ID = 0;
	uint32_t m_Count = 0;
};

struct VertexBufferElement
{
	uint32_t type = 0;
	uint32_t count = 0;
	uint8_t  normalized = 0;

	static uint32_t GetSizeOfType(uint32_t type);
};

class VertexBufferLayout
{
public:
	template<typename T>
	void Push(uint32_t count, bool instanced = false)
	{
		// static_assert(false);
	}

	template<>
	void Push<float>(uint32_t count, bool instanced)
	{
		m_Elements.push_back({ GL_FLOAT, count, GL_FALSE });
		m_Stride += count * sizeof(GLfloat);
	}

	template<>
	void Push<uint32_t>(uint32_t count, bool instanced)
	{
		m_Elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE });
		m_Stride += count * sizeof(GLuint);
	}

	template<>
	void Push<uint8_t>(uint32_t count, bool instanced)
	{
		m_Elements.push_back({ GL_UNSIGNED_BYTE, count, GL_FALSE });
		m_Stride += count * sizeof(GLbyte);
	}

	void Clear()
	{
		m_Elements.clear();
		m_Stride = 0;
	}

	inline const std::vector<VertexBufferElement>& GetElements() const { return m_Elements; }
	inline const uint32_t GetStride() const { return m_Stride; }

private:
	std::vector<VertexBufferElement> m_Elements;
	uint32_t m_Stride = 0;
};

class VertexArray
{
public:
	VertexArray();
	~VertexArray();

	void AddBuffers(const std::shared_ptr<VertexBuffer>& vbo, std::unique_ptr<IndexBuffer>& ibo, const VertexBufferLayout& layout, uint32_t attribOffset = 0);
	void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vbo, const VertexBufferLayout& layout, uint32_t attribOffset = 0);
	void AddInstancedVertexBuffer(const std::shared_ptr<VertexBuffer>& vbo, const VertexBufferLayout& layout, uint32_t attribOffset = 0) const;

	void Bind() const;
	void Unbind() const;

	inline uint32_t VertexCount() const { return m_VertexCount; }
	inline const std::unique_ptr<IndexBuffer>& GetIndexBuffer() const { return m_IBO; }

private:
	uint32_t m_ID = 0;
	uint32_t m_VertexCount = 0;
	std::unique_ptr<IndexBuffer> m_IBO;
};

class UniformBuffer
{
public:
	UniformBuffer(const void* data, uint64_t size);
	~UniformBuffer();

	void Bind() const;
	void Unbind() const;
	void BindBufferRange(uint32_t bufferIndex, uint32_t offset, uint32_t size);
	void SetData(const void* data, uint32_t size, uint32_t offset = 0) const;

private:
	uint32_t m_ID = 0;
};

struct StringReplacement
{
	std::string Pattern;
	std::string Target;
};

struct ShaderDescriptor
{
	std::string Path;
	std::vector<StringReplacement> Replacement;
};

struct ShaderSpec
{
	ShaderDescriptor Vertex;
	ShaderDescriptor Fragment;
	std::optional<ShaderDescriptor> Geometry;
};

class Shader
{
public:
	Shader(const ShaderSpec& spec);
	~Shader();

	void Bind() const;
	void Unbind() const;

	void SetUniform1i(const std::string& name, int32_t val);
	void SetUniform1f(const std::string& name, float val);
	void SetUniform2f(const std::string& name, const glm::vec2& vec);
	void SetUniform3f(const std::string& name, const glm::vec3& vec);
	void SetUniform4f(const std::string& name, const glm::vec4& vec);
	void SetUniformMat4(const std::string& name, const glm::mat4& vec);
	void SetUniformBool(const std::string& name, bool flag);

private:
	std::optional<std::string> ParseShaderSource(const std::string& path);
	uint32_t CreateShader(const std::string& vsrc, const std::string& fsrc, std::optional<std::string> gsrc);
	uint32_t CompileShader(uint32_t type, const std::string& source);
	int32_t UniformLocation(const std::string& name);

	ShaderSpec m_Spec;
	std::unordered_map<std::string, int32_t> m_UniformLocations;
	uint32_t m_ID = 0;
};

class SharedBuffer
{
public:
	SharedBuffer(const void* data, uint64_t size);
	~SharedBuffer();

	void Bind() const;
	void Unbind() const;
	void BindBufferSlot(uint32_t bufferIndex);
	void ResetData(const void* data, uint64_t size) const;
	void SetData(const void* data, uint32_t size, uint32_t offset = 0) const;

private:
	uint32_t m_ID = 0;
};

enum class RenderbufferType
{
	DEPTH,
	STENCIL,
	DEPTH_STENCIL
};

struct RenderbufferSpec
{
	RenderbufferType Type;
	glm::ivec2 Size;
};

enum class TextureFormat
{
	RGBA8,
	RGB8,
	RGBA16F,
	RGB16F,
	RG16F,
	RGB32F,
	R11_G11_B10,
};

enum class ColorAttachmentType
{
	TEX_2D,
	TEX_2D_MULTISAMPLE,
	TEX_CUBEMAP
};

struct ColorAttachmentSpec
{
	ColorAttachmentType Type;
	TextureFormat Format;
	int32_t Wrap;
	int32_t MinFilter;
	int32_t MagFilter;
	glm::ivec2 Size;
	bool GenMipmaps;
};

struct ColorAttachment
{
	uint32_t ID;
	ColorAttachmentSpec spec;
};

class Framebuffer
{
public:
	Framebuffer(uint32_t samples);
	~Framebuffer();

	void Bind() const;
	void Unbind() const;
	void BlitBuffers(uint32_t sourceAttachment, uint32_t targetAttachment, const Framebuffer& target) const;
	void ResizeRenderbuffer(const glm::uvec2& size);
	void ResizeEverything(const glm::uvec2& size);
	void FillDrawBuffers();

	void AddRenderbuffer(RenderbufferSpec spec);
	void AddColorAttachment(ColorAttachmentSpec spec);

	void BindRenderbuffer() const;
	void BindColorAttachment(uint32_t attachmentIdx, uint32_t slot = 0) const;

	void DrawToColorAttachment(uint32_t attachmentIdx, uint32_t targetAttachment, int32_t mip = 0) const;
	void DrawToCubeColorAttachment(uint32_t attachmentIdx, uint32_t targetAttachment, int32_t faceIdx, int32_t mip = 0) const;
	void ClearColorAttachment(uint32_t attachmentIdx, uint32_t mip = 0) const;

	void RemoveRenderbuffer();
	void RemoveColorAttachment(uint32_t attachmentIdx);

	inline glm::ivec2 BufferSize() const { return m_RBO_Spec.Size; }
	inline const std::vector<ColorAttachment>& ColorAttachments() const { return m_ColorAttachments; }
	inline glm::ivec2 ColorAttachmentSize(uint32_t attachmentIdx) const { return m_ColorAttachments[attachmentIdx].spec.Size; }
	inline uint32_t GetColorAttachmentID(uint32_t attachmentIdx) const { return m_ColorAttachments[attachmentIdx].ID; }
	glm::u8vec4 GetPixelAt(const glm::vec2& coords, int32_t attachmentIdx) const;
	bool IsComplete() const;

private:
	uint32_t m_ID = 0;
	uint32_t m_RenderbufferID = 0;
	RenderbufferSpec m_RBO_Spec{};

	std::vector<ColorAttachment> m_ColorAttachments;

	uint32_t m_Samples = 0;
};

class Texture
{
public:
	Texture(const std::string& path, TextureFormat format = TextureFormat::RGBA8);
	Texture(const void* data, int32_t width, int32_t height, const std::string& name, TextureFormat format = TextureFormat::RGBA8);
	Texture(uint32_t id, const std::string& name, TextureFormat format);
	~Texture();

	void SetFilter(int32_t filter);
	void SetWrap(int32_t wrap);

	void Bind(uint32_t slot = 0) const;
	void Unbind() const;

	inline int32_t GetWidth() const { return m_Width; }
	inline int32_t GetHeight() const { return m_Height; }
	inline uint32_t GetID() const { return m_ID; }
	inline const std::string& GetPath() const { return m_Path; }
	inline const std::string& Name() const { return m_Name; }

	inline int32_t Filter() const { return m_Filter; }
	inline int32_t Wrap()   const { return m_Wrap; }

private:
	uint32_t m_ID	  = 0;
	int32_t	 m_Width  = 0;
	int32_t	 m_Height = 0;
	int32_t	 m_BPP	  = 0;

	int32_t m_Filter = GL_LINEAR;
	int32_t m_Wrap   = GL_REPEAT;
	
	std::string	m_Path;
	std::string m_Name;
};