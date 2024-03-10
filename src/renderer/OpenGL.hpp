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

class Shader
{
public:
	Shader(const std::string& vs, const std::string& fs);
	Shader(const std::string& vs, const std::string& gs, const std::string& fs);
	~Shader();

	void Bind() const;
	void Unbind() const;

	void SetUniform1i(const std::string& name, int32_t val);
	void SetUniform1f(const std::string& name, float val);
	void SetUniform3f(const std::string& name, const glm::vec3& vec);
	void SetUniform4f(const std::string& name, const glm::vec4& vec);
	void SetUniformMat4(const std::string& name, const glm::mat4& vec);
	void SetUniformBool(const std::string& name, bool flag);

	bool ReloadShader();

private:
	std::optional<std::string> ShaderParse(const std::string& filepath);
	uint32_t ShaderCompile(uint32_t type, const std::string& sourceCode);
	uint32_t ShaderCreate(const std::string& vs, const std::string& fs);
	uint32_t ShaderCreate(const std::string& vs, const std::string& gs, const std::string& fs);

	int32_t GetUniformLocation(const std::string& name);

	std::string m_VertexShaderPath{};
	std::string m_FragmentShaderPath{};
	std::string m_GeometryShaderPath{};

	uint32_t m_ID = 0;
	std::unordered_map<std::string, int32_t> m_UniformLocations;
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

class Framebuffer
{
public:
	Framebuffer(const glm::uvec2& bufferSize, uint32_t samples);
	~Framebuffer();

	void Bind() const;
	void Unbind() const;
	void Resize(const glm::uvec2& bufferSize);
	void BlitBuffers(uint32_t sourceAttachmentIndex, uint32_t targetAttachmentID, const Framebuffer& target) const;

	void AddColorAttachment(GLenum format);
	void BindColorAttachment(uint32_t attachmentIndex = 0) const;
	void UnbindColorAttachment() const;
	void ClearColorAttachment(uint32_t attachmentIndex) const;

	inline uint32_t GetColorAttachmentID(uint32_t slot = 0) const { return m_ColorAttachments[slot].ID; }
	glm::u8vec4 GetPixelAt(const glm::vec2& coords, int32_t attachmentIdx) const;
	bool IsComplete() const;

private:
	struct ColorAttachmentData
	{
		uint32_t ID;
		GLenum	 Format;
	};

	uint32_t m_ID = 0;
	uint32_t m_RenderbufferID = 0;
	uint32_t m_Samples = 1;
	glm::uvec2 m_BufferSize{};
	std::vector<ColorAttachmentData> m_ColorAttachments;
};

class Texture
{
public:
	Texture(const std::string& path);
	Texture(const uint8_t* data, int32_t width, int32_t height, const std::string& name);
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

	int32_t m_Filter = GL_NEAREST;
	int32_t m_Wrap   = GL_REPEAT;
	
	std::string	m_Path;
	std::string m_Name;
};