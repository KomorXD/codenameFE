#pragma once

#include "Layer.hpp"
#include "../renderer/Camera.hpp"
#include "../scenes/Scene.hpp"
#include "../scenes/Entity.hpp"
#include "../renderer/Renderer.hpp"

#include <memory>

class Framebuffer;
class MultisampledFramebuffer;

class EditorLayer : public Layer
{
public:
	EditorLayer();

	virtual void OnAttach()			override;
	virtual void OnEvent(Event& ev) override;
	virtual void OnUpdate(float ts) override;
	virtual void OnTick()			override;
	virtual void OnRender()			override;

private:
	void RenderScenePanel();
	void RenderViewport();
	void RenderEntityData();
	void RenderGizmo();

	Camera m_EditorCamera;
	Scene  m_Scene;
	Entity m_SelectedEntity;
	Entity m_CopiedEntity;

	glm::vec4 m_BgColor = { 0.3f, 0.4f, 0.5f, 1.0f };

	int32_t m_GizmoOp = -1;
	int32_t m_GizmoMode;
	bool m_LockFocus    = false;
	bool m_IsGizmoUsed  = false;

	bool m_DrawWireframe = false;

	bool m_ViewportHovered = false;
	bool m_ViewportFocused = false;

	std::unique_ptr<Framebuffer> m_MainFB;
	std::unique_ptr<Framebuffer> m_ScreenFB;
	std::shared_ptr<CubemapFramebuffer> m_Skybox;

	RendererStats m_Stats{};
};