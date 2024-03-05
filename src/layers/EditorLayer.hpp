#pragma once

#include "Layer.hpp"
#include "../renderer/Camera.hpp"
#include "../scenes/Scene.hpp"
#include "../scenes/Entity.hpp"

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
	void RenderViewport();

	Camera m_EditorCamera;
	Scene m_Scene;
	Entity m_SelectedEntity;

	bool m_ViewportHovered = false;

	std::unique_ptr<Framebuffer> m_ScreenFB;
	std::unique_ptr<Framebuffer> m_TargetFB;
	std::unique_ptr<MultisampledFramebuffer> m_MainMFB;
	std::unique_ptr<Framebuffer> m_PickerFB;
};