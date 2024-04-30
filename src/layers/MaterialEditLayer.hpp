#pragma once

#include "Layer.hpp"
#include "../renderer/Camera.hpp"
#include "../scenes/Scene.hpp"
#include "../scenes/Entity.hpp"

#include <memory>

class OldFramebuffer;
class MultisampledFramebuffer;

class MaterialEditLayer : public Layer
{
public:
	MaterialEditLayer(std::vector<Entity>& mainEntities, std::shared_ptr<Framebuffer> skyboxFBO);
	MaterialEditLayer(std::vector<Entity>& mainEntities, int32_t materialID, std::shared_ptr<Framebuffer> skyboxFBO);

	virtual void OnAttach()			override;
	virtual void OnEvent(Event& ev) override;
	virtual void OnUpdate(float ts) override;
	virtual void OnTick()			override;
	virtual void OnRender()			override;

private:
	void RenderPanel();

	Camera m_Camera;
	Scene  m_Scene;
	Entity m_SampleEnt;
	Entity m_LightEnt;
	
	std::vector<Entity>& m_MainEntities;
	glm::vec3 m_BgColor = { 0.3f, 0.4f, 0.5f };

	bool m_ViewportHovered = false;
	bool m_ViewportFocused = false;

	float m_BloomStrength = 0.04f;
	float m_BloomThreshold = 1.0f;
	bool m_UseBloom = true;

	std::unique_ptr<Framebuffer> m_MainFB;
	std::unique_ptr<Framebuffer> m_ScreenFB;
	std::shared_ptr<Framebuffer> m_SkyboxFBO;
};