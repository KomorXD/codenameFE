#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

class Shader;
class VertexArray;

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

	static void SceneBegin();
	static void SceneEnd();
	static void Flush();

	static void ClearColor(const glm::vec4& color);
	static void Clear();

	static void DrawQuad(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);

	static void DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t count = 0);

private:
	static void StartBatch();
	static void NextBatch();

	static glm::mat4 s_ViewProjection;
	static glm::mat4 s_Projection;
	static glm::mat4 s_View;
};