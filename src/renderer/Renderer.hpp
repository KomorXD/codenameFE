#pragma once

#include <memory>
#include <string>

#include "OpenGL.hpp"
#include "LightCasters.hpp"

class Shader;
class VertexArray;
class Camera;

struct Viewport
{
	int32_t StartX = 0;
	int32_t StartY = 0;
	int32_t Width = 0;
	int32_t Height = 0;
};

class Renderer
{
public:
	static void Init();
	static void Shutdown();

	static void ReloadShaders();

	static void OnWindowResize(const Viewport& newViewport);

	static void SceneBegin(Camera& camera);
	static void SceneEnd();
	static void Flush();

	static void ClearColor(const glm::vec4& color);
	static void Clear(uint32_t bitfield = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
	static void DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
	static void DrawQuad(const glm::vec3& position, const glm::vec3& size, const Texture& texture);
	static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
	static void DrawCube(const glm::vec3& position, const glm::vec3& size, const Texture& texture);

	static void DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawIndexedInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawArrays(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t vertexCount, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawArraysInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawScreenQuad();

	static void AddDirectionalLight(const DirectionalLight& light);
	static void AddPointLight(const PointLight& light);
	static void AddSpotLight(const SpotLight& light);

	static void SetBlur(bool enabled);
	static void RenderDefault();
	static void RenderDepth();

	static Viewport CurrentViewport();

private:
	static void StartBatch();
	static void NextBatch();

	static Camera* s_ActiveCamera;
	static Viewport s_Viewport;
};