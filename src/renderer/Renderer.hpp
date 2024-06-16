#pragma once

#include <memory>
#include <string>

#include "OpenGL.hpp"
#include "../scenes/Components.hpp"

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

struct RendererStats
{
	uint32_t RenderPassDrawCalls = 0;
	uint32_t DrawCalls = 0;
	uint32_t ObjectsRendered = 0;
	uint32_t DirLightCascadesPassed = 0;
	uint32_t PointLightFacesShadowPassed = 0;
	uint32_t SpotlightFacesShadowPassed = 0;
	float DirLightShadowPassTime = 0.0f;
	float PointLightShadowPassTime = 0.0f;
	float SpotlightShadowPassTime = 0.0f;
};

struct G_BuffersIDs
{
	uint32_t G_Position;
	uint32_t G_Normal;
	uint32_t G_Color;
	uint32_t G_Material;
	uint32_t G_Lights;
};

enum class RenderMode
{
	FORWARD = 0,
	DEFERRED,
	FLAT_SHADING,

	COUNT
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

	static void BeginShadowMapPass();
	static void EndShadowMapPass();

	static void ResetStats();
	static RendererStats Stats();

	static void ClearColor(const glm::vec4& color);
	static void Clear(uint32_t bitfield = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
	static void DrawCube(const glm::mat4& transform, const glm::vec4& color);

	static void SubmitMesh(const glm::mat4& transform, const MeshComponent& mesh, const Material& material, int32_t entityID);

	static void DrawIndexed(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawIndexedInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawArrays(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t vertexCount, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawArraysInstanced(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vao, uint32_t instances, uint32_t primitiveType = GL_TRIANGLES);
	static void DrawScreenQuad();

	static void SetRenderMode(RenderMode pipeline);
	static void SetTargetFBO(std::shared_ptr<Framebuffer>& fbo);

	static void Bloom(std::shared_ptr<Framebuffer> hdrFBO);
	static void SetBloomStrength(float strength);
	static void SetBloomThreshold(float threshold);

	static void SetOffsetsRadius(float radius);

	static std::shared_ptr<Framebuffer> CreateEnvCubemap(std::shared_ptr<Texture> hdrEnvMap, const glm::uvec2& faceSize = { 512, 512 });
	static void DrawSkybox(std::shared_ptr<Framebuffer> cfb);

	static void AddDirectionalLight(const TransformComponent& transform, const DirectionalLightComponent& light);
	static void AddPointLight(const glm::vec3& position, const PointLightComponent& light);
	static void AddSpotLight(const TransformComponent& transform, const SpotLightComponent& light);

	static void EnableStencil();
	static void DisableStencil();
	static void SetStencilFunc(uint32_t func, int32_t ref, uint32_t mask);
	static void SetStencilMask(uint32_t mask);

	static void EnableDepthTest();
	static void DisableDepthTest();

	static void EnableFaceCulling();
	static void DisableFaceCulling();

	static void SetWireframe(bool enabled);

	static Viewport CurrentViewport();
	static G_BuffersIDs G_Buffers();

private:
	static void StartBatch();
	static void NextBatch();

	static void ForwardRender();
	static void DeferredRender();

	static Camera* s_ActiveCamera;
	static Viewport s_Viewport;
	static std::shared_ptr<Framebuffer> s_TargetFBO;
};