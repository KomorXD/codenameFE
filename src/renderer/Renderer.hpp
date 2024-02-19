#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

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
	static void Clear();

	static void DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
	static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);

	static void DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t count, uint32_t primitiveType);
	static void DrawArrays(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t vertexCount, uint32_t primitiveType);
	static void DrawArraysInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances);

private:
	static void StartBatch();
	static void NextBatch();

	static glm::mat4 s_ViewProjection;
	static glm::mat4 s_Projection;
	static glm::mat4 s_View;
};