#pragma once

#include "Layer.hpp"

class EditorLayer : public Layer
{
public:
	EditorLayer();

	virtual void OnAttach()			override;
	virtual void OnEvent(Event& ev) override;
	virtual void OnUpdate(float ts) override;
	virtual void OnTick()			override;
	virtual void OnRender()			override;
};