#pragma once

#include "Layer.hpp"
#include "../renderer/Camera.hpp"

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
	Camera m_EditorCamera;

	std::unique_ptr<Framebuffer> m_ScreenFB;
	std::unique_ptr<Framebuffer> m_TargetFB;
	std::unique_ptr<MultisampledFramebuffer> m_MainFB;
};